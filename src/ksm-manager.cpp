/**
 * Copyright (c) 2020 ~ 2021 KylinSec Co., Ltd. 
 * kiran-session-manager is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2. 
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2 
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, 
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, 
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.  
 * See the Mulan PSL v2 for more details.  
 * 
 * Author:     tangjie02 <tangjie02@kylinos.com.cn>
 */

#include "src/ksm-manager.h"
#include <json/json.h>
#include "src/client/ksm-client-manager.h"
#include "src/ksm-app-manager.h"
#include "src/ksm-utils.h"

namespace Kiran
{
// 在一个阶段等待所有应用的最长时间(秒)
#define KSM_PHASE_STARTUP_TIMEOUT 30

KSMManager::KSMManager(KSMAppManager *app_manager,
                       KSMClientManager *client_manager) : app_manager_(app_manager),
                                                           client_manager_(client_manager),
                                                           current_phase_(KSMPhase::KSM_PHASE_IDLE),
                                                           power_action_(KSMPowerAction::KSM_POWER_ACTION_NONE),
                                                           dbus_connect_id_(0),
                                                           object_register_id_(0)
{
    this->settings_ = Gio::Settings::create(KSM_SCHEMA_ID);
    this->presence_ = std::make_shared<KSMPresence>();
}

KSMManager::~KSMManager()
{
    if (this->dbus_connect_id_)
    {
        Gio::DBus::unown_name(this->dbus_connect_id_);
    }
}

KSMManager *KSMManager::instance_ = nullptr;
void KSMManager::global_init(KSMAppManager *app_manager,
                             KSMClientManager *client_manager)
{
    instance_ = new KSMManager(app_manager, client_manager);
    instance_->init();
}

void KSMManager::start()
{
    this->current_phase_ = KSMPhase::KSM_PHASE_INITIALIZATION;
    this->process_phase();
}

void KSMManager::RegisterClient(const Glib::ustring &app_id,
                                const Glib::ustring &client_startup_id,
                                MethodInvocation &invocation)
{
    KLOG_PROFILE("app id: %s, startup id: %s.", app_id.c_str(), client_startup_id.c_str());

    if (this->current_phase_ > KSMPhase::KSM_PHASE_RUNNING)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_PHASE_CANNOT_REGISTER, KSMUtils::phase_enum2str(this->current_phase_));
    }

    auto client = this->client_manager_->add_client_dbus(client_startup_id);

    if (!client)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_CLIENT_ALREADY_REGISTERED, client_startup_id);
    }

    invocation.ret(client->get_object_path());
}

void KSMManager::Inhibit(const Glib::ustring &app_id,
                         guint32 toplevel_xid,
                         const Glib::ustring &reason,
                         guint32 flags,
                         MethodInvocation &invocation)
{
    KLOG_PROFILE("app id: %s, toplevel xid: %d, reason: %s, flags: %d.",
                 app_id.c_str(),
                 toplevel_xid,
                 reason.c_str(),
                 flags);

    auto inhibitor = this->inhibitors_.add_inhibitor(app_id, toplevel_xid, reason, flags);

    if (!inhibitor)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_GEN_UNIQUE_COOKIE_FAILED);
    }

    invocation.ret(inhibitor->cookie);
}

void KSMManager::Uninhibit(guint32 inhibit_cookie,
                           MethodInvocation &invocation)
{
    KLOG_PROFILE("inhibit cookie: %u.", inhibit_cookie);

    auto inhibitor = this->inhibitors_.get_inhibitor(inhibit_cookie);
    if (!inhibitor)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_INHIBITOR_NOTFOUND);
    }
    this->inhibitors_.delete_inhibitor(inhibit_cookie);
    invocation.ret();
}

void KSMManager::IsInhibited(guint32 flags,
                             MethodInvocation &invocation)
{
    KLOG_PROFILE("flags: %u.", flags);
    invocation.ret(this->inhibitors_.has_inhibitor(flags));
}

void KSMManager::GetInhibitors(MethodInvocation &invocation)
{
    KLOG_PROFILE("");

    Json::Value values;
    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";

    try
    {
        int32_t i = 0;
        for (auto inhibitor : this->inhibitors_.get_inhibitors())
        {
            values[i]["cookie"] = inhibitor->cookie;
            values[i]["app_id"] = inhibitor->app_id;
            values[i]["toplevel_xid"] = inhibitor->toplevel_xid;
            values[i]["reason"] = inhibitor->reason;
            values[i]["flags"] = inhibitor->flags;
        }

        auto retval = Json::writeString(wbuilder, values);
        invocation.ret(Glib::ustring(retval));
    }
    catch (const std::exception &e)
    {
        KLOG_WARNING("%s.", e.what());
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_GET_INHIBITORS_FAILED);
    }
}

void KSMManager::Shutdown(MethodInvocation &invocation)
{
    KLOG_PROFILE("");

    if (this->current_phase_ > KSMPhase::KSM_PHASE_RUNNING)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_PHASE_INVALID);
    }

    if (!this->power_.can_power_action(KSMPowerAction::KSM_POWER_ACTION_SHUTDOWN))
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_POWER_ACTION_UNSUPPORTED);
    }

    this->power_action_ = KSMPowerAction::KSM_POWER_ACTION_SHUTDOWN;
    this->start_next_phase();
    invocation.ret();
}

void KSMManager::RequestShutdown(MethodInvocation &invocation)
{
    KLOG_PROFILE("");

    this->Shutdown(invocation);
}

void KSMManager::CanShutdown(MethodInvocation &invocation)
{
    KLOG_PROFILE("");
    invocation.ret(this->power_.can_power_action(KSMPowerAction::KSM_POWER_ACTION_SHUTDOWN));
}

void KSMManager::Logout(guint32 mode, MethodInvocation &invocation)
{
    KLOG_PROFILE("");

    if (this->current_phase_ > KSMPhase::KSM_PHASE_RUNNING)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_PHASE_INVALID);
    }

    if (!this->power_.can_power_action(KSMPowerAction::KSM_POWER_ACTION_LOGOUT))
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_POWER_ACTION_UNSUPPORTED);
    }

    this->power_action_ = KSMPowerAction::KSM_POWER_ACTION_LOGOUT;
    this->start_next_phase();
    invocation.ret();
}

void KSMManager::CanLogout(MethodInvocation &invocation)
{
    KLOG_PROFILE("");
    invocation.ret(true);
}

void KSMManager::Reboot(MethodInvocation &invocation)
{
    KLOG_PROFILE("");

    if (this->current_phase_ > KSMPhase::KSM_PHASE_RUNNING)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_PHASE_INVALID);
    }

    if (!this->power_.can_power_action(KSMPowerAction::KSM_POWER_ACTION_REBOOT))
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_POWER_ACTION_UNSUPPORTED);
    }

    this->power_action_ = KSMPowerAction::KSM_POWER_ACTION_REBOOT;
    this->start_next_phase();
    invocation.ret();
}

void KSMManager::RequestReboot(MethodInvocation &invocation)
{
    KLOG_PROFILE("");
    this->Reboot(invocation);
}

void KSMManager::CanReboot(MethodInvocation &invocation)
{
    KLOG_PROFILE("");
    invocation.ret(this->power_.can_power_action(KSMPowerAction::KSM_POWER_ACTION_REBOOT));
}

void KSMManager::init()
{
    this->power_.init();
    this->presence_->init();

    auto resource = Gio::Resource::create_from_file(KSM_INSTALL_DATADIR "/kiran-session-manager.gresource");
    resource->register_global();

    this->client_manager_->signal_client_added().connect(sigc::mem_fun(this, &KSMManager::on_client_added_cb));
    this->client_manager_->signal_client_deleted().connect(sigc::mem_fun(this, &KSMManager::on_client_deleted_cb));
    this->client_manager_->signal_shutdown_canceled().connect(sigc::mem_fun(this, &KSMManager::on_xsmp_shutdown_canceled_cb));
    this->client_manager_->signal_end_session_phase2_request().connect(sigc::mem_fun(this, &KSMManager::on_end_session_phase2_request_cb));
    this->client_manager_->signal_end_session_response().connect(sigc::mem_fun(this, &KSMManager::on_end_session_response_cb));

    this->dbus_connect_id_ = Gio::DBus::own_name(Gio::DBus::BUS_TYPE_SESSION,
                                                 KSM_DBUS_NAME,
                                                 sigc::mem_fun(this, &KSMManager::on_bus_acquired),
                                                 sigc::mem_fun(this, &KSMManager::on_name_acquired),
                                                 sigc::mem_fun(this, &KSMManager::on_name_lost));
}

void KSMManager::process_phase()
{
    auto phase_str = KSMUtils::phase_enum2str(this->current_phase_);
    KLOG_PROFILE("phase: %s.", phase_str.c_str());

    this->waiting_apps_.clear();
    this->waiting_apps_timeout_id_.disconnect();
    this->waiting_clients_.clear();
    this->waiting_clients_timeout_id_.disconnect();

    this->PhaseChanged_signal.emit(this->current_phase_);

    switch (this->current_phase_)
    {
    case KSMPhase::KSM_PHASE_INITIALIZATION:
    case KSMPhase::KSM_PHASE_WINDOW_MANAGER:
    case KSMPhase::KSM_PHASE_PANEL:
    case KSMPhase::KSM_PHASE_DESKTOP:
    case KSMPhase::KSM_PHASE_APPLICATION:
        this->process_phase_startup();
        break;
    case KSMPhase::KSM_PHASE_RUNNING:
        break;
    case KSMPhase::KSM_PHASE_QUERY_END_SESSION:
        this->process_phase_query_end_session();
        break;
    case KSMPhase::KSM_PHASE_END_SESSION_PHASE1:
        this->process_phase_end_session_phase1();
        break;
    case KSMPhase::KSM_PHASE_END_SESSION_PHASE2:
        this->process_phase_end_session_phase2();
    case KSMPhase::KSM_PHASE_EXIT:
        this->process_phase_exit();
        break;
    default:
        break;
    }
}

void KSMManager::process_phase_startup()
{
    KLOG_PROFILE("");

    auto apps = this->app_manager_->start_apps(this->current_phase_);

    // 在KSM_PHASE_APPLICATION阶段前启动的应用在运行后需要（通过dbus或者xsmp规范）告知会话管理自己已经启动成功，这样会话管理才会进入下一个阶段。
    if (this->current_phase_ < KSMPhase::KSM_PHASE_APPLICATION)
    {
        this->waiting_apps_ = std::move(apps);
    }

    // 一些应用需要延时执行或者需要等待其启动完毕的信号
    if (this->waiting_apps_.size() > 0)
    {
        if (this->current_phase_ < KSMPhase::KSM_PHASE_APPLICATION)
        {
            auto timeout = Glib::MainContext::get_default()->signal_timeout();
            this->waiting_apps_timeout_id_ = timeout.connect_seconds(sigc::mem_fun(this, &KSMManager::on_phase_startup_timeout),
                                                                     KSM_PHASE_STARTUP_TIMEOUT);
        }
    }
    else
    {
        this->start_next_phase();
    }
}

void KSMManager::process_phase_query_end_session()
{
    KLOG_PROFILE("");

    for (auto client : this->client_manager_->get_clients())
    {
        if (!client->query_end_session(true))
        {
            KLOG_WARNING("Failed to query client: %s.", client->get_id().c_str());
        }
        else
        {
            this->waiting_clients_.push_back(client);
        }
    }
    if (this->waiting_clients_.size() > 0)
    {
        auto timeout = Glib::MainContext::get_default()->signal_timeout();
        this->waiting_clients_timeout_id_ = timeout.connect_seconds(sigc::bind(sigc::mem_fun(this, &KSMManager::on_waiting_session_timeout),
                                                                               sigc::mem_fun(this, &KSMManager::query_end_session_complete)),
                                                                    1);
    }
    else
    {
        this->start_next_phase();
    }
}

void KSMManager::query_end_session_complete()
{
    KLOG_PROFILE("");

    if (this->current_phase_ != KSMPhase::KSM_PHASE_QUERY_END_SESSION)
    {
        KLOG_WARNING("The phase is error. phase: %s.", KSMUtils::phase_enum2str(this->current_phase_).c_str());
        return;
    }

    this->waiting_clients_.clear();
    this->waiting_clients_timeout_id_.disconnect();

    // TODO: 暂时不支持抑制器功能
    this->start_next_phase();
    return;

    // 如果不存在退出会话的抑制器，则直接进入下一个阶段
    if (!this->inhibitors_.has_inhibitor(KSMInhibitorFlag::KSM_INHIBITOR_FLAG_QUIT))
    {
        this->start_next_phase();
        return;
    }

    if (this->quit_dialog_)
    {
        this->quit_dialog_->present();
        return;
    }

    this->quit_dialog_ = KSMQuitDialog::create(this->power_action_);
    this->quit_dialog_->signal_response().connect(sigc::mem_fun(this, &KSMManager::on_quit_dialog_response));
    this->quit_dialog_->show_all();
}

void KSMManager::cancel_end_session()
{
    KLOG_PROFILE("");

    RETURN_IF_TRUE(this->current_phase_ < KSMPhase::KSM_PHASE_QUERY_END_SESSION);

    for (auto client : this->client_manager_->get_clients())
    {
        if (!client->cancel_end_session())
        {
            KLOG_WARNING("Failed to cancel end session for the client: %s.", client->get_id().c_str());
        }
    }

    this->quit_dialog_ = nullptr;
    this->power_action_ = KSMPowerAction::KSM_POWER_ACTION_NONE;

    // 回到运行阶段
    this->current_phase_ = KSMPhase::KSM_PHASE_RUNNING;
    // 不能在process_phase中清理第二阶段缓存客户端，因为在调用process_phase_end_session_phase2函数时需要使用
    this->phase2_request_clients_.clear();
    this->process_phase();
}

void KSMManager::process_phase_end_session_phase1()
{
    for (auto client : this->client_manager_->get_clients())
    {
        if (!client->end_session(false))
        {
            KLOG_WARNING("Failed to end client: %s.", client->get_id().c_str());
        }
        else
        {
            this->waiting_clients_.push_back(client);
        }
    }

    if (this->waiting_clients_.size() > 0)
    {
        auto timeout = Glib::MainContext::get_default()->signal_timeout();
        this->waiting_clients_timeout_id_ = timeout.connect_seconds(sigc::bind(sigc::mem_fun(this, &KSMManager::on_waiting_session_timeout),
                                                                               sigc::mem_fun(this, &KSMManager::start_next_phase)),
                                                                    30);
    }
    else
    {
        this->start_next_phase();
    }
}

void KSMManager::process_phase_end_session_phase2()
{
    // 如果有客户端需要进行第二阶段的数据保存操作，则告知这些客户端现在可以开始进行了
    if (this->phase2_request_clients_.size() > 0)
    {
        for (auto client : this->phase2_request_clients_)
        {
            if (!client->end_session_phase2())
            {
                KLOG_WARNING("Failed to end session phase2 for the client: %s.", client->get_id().c_str());
            }
            else
            {
                this->waiting_clients_.push_back(client);
            }
        }
        this->phase2_request_clients_.clear();
    }

    if (this->waiting_clients_.size() > 0)
    {
        auto timeout = Glib::MainContext::get_default()->signal_timeout();
        this->waiting_clients_timeout_id_ = timeout.connect_seconds(sigc::bind(sigc::mem_fun(this, &KSMManager::on_waiting_session_timeout),
                                                                               sigc::mem_fun(this, &KSMManager::start_next_phase)),
                                                                    30);
    }
    else
    {
        this->start_next_phase();
    }
}

void KSMManager::process_phase_exit()
{
    for (auto client : this->client_manager_->get_clients())
    {
        if (!client->stop())
        {
            KLOG_WARNING("Failed to stop client: %s.", client->get_id().c_str());
        }
    }
    this->start_next_phase();
}

void KSMManager::start_next_phase()
{
    bool start_next_phase = TRUE;

    switch (this->current_phase_)
    {
    case KSMPhase::KSM_PHASE_IDLE:
    case KSMPhase::KSM_PHASE_INITIALIZATION:
    case KSMPhase::KSM_PHASE_WINDOW_MANAGER:
    case KSMPhase::KSM_PHASE_PANEL:
    case KSMPhase::KSM_PHASE_DESKTOP:
    case KSMPhase::KSM_PHASE_APPLICATION:
        break;
    case KSMPhase::KSM_PHASE_RUNNING:
        break;
    case KSMPhase::KSM_PHASE_QUERY_END_SESSION:
        break;
    case KSMPhase::KSM_PHASE_END_SESSION_PHASE1:
        // if (auto_save_is_enabled(manager))
        //     maybe_save_session(manager);
        break;
    case KSMPhase::KSM_PHASE_EXIT:
        this->quit_session();
        break;
    default:
        break;
    }

    if (start_next_phase)
    {
        this->current_phase_ = (KSMPhase)(this->current_phase_ + 1);
        auto phase_str = KSMUtils::phase_enum2str(this->current_phase_);
        KLOG_DEBUG("Start next phase: %s.", phase_str.c_str());
        this->process_phase();
    }
    else
    {
        auto phase_str = KSMUtils::phase_enum2str(this->current_phase_);
        KLOG_DEBUG("Keep current phase: %s.", phase_str.c_str());
    }
}

void KSMManager::quit_session()
{
    this->power_.do_power_action(this->power_action_);
}

void KSMManager::on_app_startup_finished(std::shared_ptr<KSMApp> app)
{
    RETURN_IF_FALSE(app);

    auto iter = std::remove_if(this->waiting_apps_.begin(),
                               this->waiting_apps_.end(),
                               [app](std::shared_ptr<KSMApp> item) -> bool {
                                   return item->get_app_id() == app->get_app_id();
                               });

    this->waiting_apps_.erase(iter, this->waiting_apps_.end());

    if (this->waiting_apps_.size() == 0 &&
        app->get_phase() < KSMPhase::KSM_PHASE_APPLICATION)
    {
        if (this->waiting_apps_timeout_id_)
        {
            this->waiting_apps_timeout_id_.disconnect();
        }
        this->start_next_phase();
    }
}

bool KSMManager::on_phase_startup_timeout()
{
    switch (this->current_phase_)
    {
    case KSMPhase::KSM_PHASE_IDLE:
    case KSMPhase::KSM_PHASE_INITIALIZATION:
    case KSMPhase::KSM_PHASE_WINDOW_MANAGER:
    case KSMPhase::KSM_PHASE_PANEL:
    case KSMPhase::KSM_PHASE_DESKTOP:
    case KSMPhase::KSM_PHASE_APPLICATION:
    {
        for (auto app : this->waiting_apps_)
        {
            KLOG_WARNING("Wait app %s startup timeout.", app->get_app_id().c_str());
        }
        break;
    }
    default:
        break;
    }

    this->waiting_apps_timeout_id_.disconnect();
    this->start_next_phase();
    return false;
}

bool KSMManager::on_waiting_session_timeout(std::function<void(void)> phase_complete_callback)
{
    for (auto client : this->waiting_clients_)
    {
        KLOG_WARNING("The client %s doesn't response message. phase: %s",
                     client->get_id().c_str(),
                     KSMUtils::phase_enum2str(this->current_phase_).c_str());
    }

    phase_complete_callback();
    return false;
}

void KSMManager::on_quit_dialog_response(int32_t response_id)
{
    KLOG_PROFILE("response id: %d.", response_id);

    auto action = this->quit_dialog_->get_power_action();
    this->quit_dialog_ = nullptr;

    switch (response_id)
    {
    case Gtk::RESPONSE_CANCEL:
    case Gtk::RESPONSE_NONE:
    case Gtk::RESPONSE_DELETE_EVENT:
        this->cancel_end_session();
        break;
    case Gtk::RESPONSE_ACCEPT:
        this->start_next_phase();
        break;
    default:
        break;
    }
}

void KSMManager::on_client_added_cb(std::shared_ptr<KSMClient> client)
{
    KLOG_PROFILE("");

    RETURN_IF_FALSE(client);
    auto app = this->app_manager_->get_app_by_startup_id(client->get_id());
    if (!app)
    {
        KLOG_WARNING("Not found app for the client %s.", client->get_id().c_str());
        return;
    }
    KLOG_DEBUG("The new client %s match the app %s.", client->get_id().c_str(), app->get_app_id().c_str());
    this->on_app_startup_finished(app);
}

void KSMManager::on_client_deleted_cb(std::shared_ptr<KSMClient> client)
{
    RETURN_IF_FALSE(client);
    KLOG_DEBUG("client: %s.", client->get_id());
    // 客户端断开连接或者异常退出后无法再响应会话管理的请求，因此这里主动调用一次客户端响应回调函数来处理该客户端
    this->on_end_session_response_cb(client);
}

void KSMManager::on_xsmp_shutdown_canceled_cb(std::shared_ptr<KSMClient> client)
{
    KLOG_PROFILE("client: %s.", client->get_id().c_str());

    this->cancel_end_session();
}

void KSMManager::on_end_session_phase2_request_cb(std::shared_ptr<KSMClient> client)
{
    KLOG_PROFILE("client: %s.", client->get_id().c_str());

    // 需要第一阶段结束的所有客户端响应后才能进入第二阶段，因此这里先缓存第二阶段请求的客户端
    this->phase2_request_clients_.push_back(client);
}

void KSMManager::on_end_session_response_cb(std::shared_ptr<KSMClient> client)
{
    KLOG_PROFILE("current phase: %s",
                 KSMUtils::phase_enum2str(this->current_phase_).c_str());

    RETURN_IF_FALSE(client);
    RETURN_IF_TRUE(this->current_phase_ < KSMPhase::KSM_PHASE_QUERY_END_SESSION);

    auto iter = std::remove_if(this->waiting_clients_.begin(),
                               this->waiting_clients_.end(),
                               [client](std::shared_ptr<KSMClient> item) {
                                   return client->get_id() == item->get_id();
                               });
    this->waiting_clients_.erase(iter, this->waiting_clients_.end());

    if (waiting_clients_.size() == 0)
    {
        switch (this->current_phase_)
        {
        case KSMPhase::KSM_PHASE_QUERY_END_SESSION:
            this->query_end_session_complete();
            break;
        case KSMPhase::KSM_PHASE_END_SESSION_PHASE1:
        case KSMPhase::KSM_PHASE_END_SESSION_PHASE2:
            this->start_next_phase();
            break;
        default:
            KLOG_WARNING("The phase is invalid. current phase: %d.", this->current_phase_);
            break;
        }
    }
}

void KSMManager::on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name)
{
    if (!connect)
    {
        KLOG_WARNING("Failed to connect dbus. name: %s", name.c_str());
        return;
    }
    try
    {
        this->object_register_id_ = this->register_object(connect, KSM_DBUS_OBJECT_PATH);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("Register object_path %s fail: %s.", KSM_DBUS_OBJECT_PATH, e.what().c_str());
    }
}

void KSMManager::on_name_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name)
{
    KLOG_DEBUG("Success to register dbus name: %s", name.c_str());
}

void KSMManager::on_name_lost(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name)
{
    KLOG_WARNING("Failed to register dbus name: %s", name.c_str());
}

}  // namespace Kiran