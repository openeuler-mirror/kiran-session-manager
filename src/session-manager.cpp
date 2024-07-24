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

#include "src/session-manager.h"
#include <glib/gi18n.h>
#include <json/json.h>
#include "src/app/app-manager.h"
#include "src/client/client-manager.h"
#include "src/inhibitor-manager.h"
#include "src/ui/exit-query-window.h"
#include "src/utils.h"

namespace Kiran
{
namespace Daemon
{
// 在一个阶段等待所有应用的最长时间(秒)
#define KSM_PHASE_STARTUP_TIMEOUT 30

SessionManager::SessionManager(AppManager *app_manager,
                               ClientManager *client_manager,
                               InhibitorManager *inhibitor_manager) : app_manager_(app_manager),
                                                                      client_manager_(client_manager),
                                                                      inhibitor_manager_(inhibitor_manager),
                                                                      current_phase_(KSMPhase::KSM_PHASE_IDLE),
                                                                      power_action_(PowerAction::POWER_ACTION_NONE),
                                                                      dbus_connect_id_(0),
                                                                      object_register_id_(0)
{
    this->settings_ = Gio::Settings::create(KSM_SCHEMA_ID);

    this->presence_ = std::make_shared<Presence>();
}

SessionManager::~SessionManager()
{
    if (this->dbus_connect_id_)
    {
        Gio::DBus::unown_name(this->dbus_connect_id_);
    }
}

SessionManager *SessionManager::instance_ = nullptr;
void SessionManager::global_init(AppManager *app_manager,
                                 ClientManager *client_manager,
                                 InhibitorManager *inhibitor_manager)
{
    instance_ = new SessionManager(app_manager, client_manager, inhibitor_manager);
    instance_->init();
}

void SessionManager::start()
{
    this->current_phase_ = KSMPhase::KSM_PHASE_INITIALIZATION;
    this->process_phase();
}

void SessionManager::RegisterClient(const Glib::ustring &app_id,
                                    const Glib::ustring &client_startup_id,
                                    MethodInvocation &invocation)
{
    KLOG_PROFILE("app id: %s, startup id: %s.", app_id.c_str(), client_startup_id.c_str());

    if (this->current_phase_ > KSMPhase::KSM_PHASE_RUNNING)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_PHASE_CANNOT_REGISTER, Utils::phase_enum2str(this->current_phase_));
    }

    auto client = this->client_manager_->add_client_dbus(client_startup_id, invocation.getMessage()->get_sender(), app_id);

    if (!client)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_CLIENT_ALREADY_REGISTERED, client_startup_id);
    }

    invocation.ret(client->get_object_path());
}

void SessionManager::Inhibit(const Glib::ustring &app_id,
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

    auto inhibitor = this->inhibitor_manager_->add_inhibitor(app_id, toplevel_xid, reason, flags);

    if (!inhibitor)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_GEN_UNIQUE_COOKIE_FAILED);
    }

    invocation.ret(inhibitor->cookie);
}

void SessionManager::Uninhibit(guint32 inhibit_cookie,
                               MethodInvocation &invocation)
{
    KLOG_PROFILE("inhibit cookie: %u.", inhibit_cookie);

    auto inhibitor = InhibitorManager::get_instance()->get_inhibitor(inhibit_cookie);
    if (!inhibitor)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_INHIBITOR_NOTFOUND);
    }
    InhibitorManager::get_instance()->delete_inhibitor(inhibit_cookie);
    invocation.ret();
}

void SessionManager::IsInhibited(guint32 flags,
                                 MethodInvocation &invocation)
{
    KLOG_PROFILE("flags: %u.", flags);
    invocation.ret(InhibitorManager::get_instance()->has_inhibitor(flags));
}

void SessionManager::GetInhibitor(guint32 cookie, MethodInvocation &invocation)
{
    KLOG_PROFILE("");

    Json::Value values;
    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";

    try
    {
        auto inhibitor = InhibitorManager::get_instance()->get_inhibitor(cookie);

        if (inhibitor)
        {
            values[KSM_INHIBITOR_JK_COOKIE] = inhibitor->cookie;
            values[KSM_INHIBITOR_JK_APP_ID] = inhibitor->app_id;
            values[KSM_INHIBITOR_JK_TOPLEVEL_XID] = inhibitor->toplevel_xid;
            values[KSM_INHIBITOR_JK_REASON] = inhibitor->reason;
            values[KSM_INHIBITOR_JK_FLAGS] = inhibitor->flags;
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

void SessionManager::GetInhibitors(MethodInvocation &invocation)
{
    KLOG_PROFILE("");

    Json::Value values;
    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";

    try
    {
        int32_t i = 0;
        for (auto inhibitor : this->inhibitor_manager_->get_inhibitors())
        {
            values[i][KSM_INHIBITOR_JK_COOKIE] = inhibitor->cookie;
            values[i][KSM_INHIBITOR_JK_APP_ID] = inhibitor->app_id;
            values[i][KSM_INHIBITOR_JK_TOPLEVEL_XID] = inhibitor->toplevel_xid;
            values[i][KSM_INHIBITOR_JK_REASON] = inhibitor->reason;
            values[i][KSM_INHIBITOR_JK_FLAGS] = inhibitor->flags;
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

void SessionManager::Shutdown(MethodInvocation &invocation)
{
    KLOG_PROFILE("");

    if (this->current_phase_ > KSMPhase::KSM_PHASE_RUNNING)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_PHASE_INVALID);
    }

    if (!this->power_.can_power_action(PowerAction::POWER_ACTION_SHUTDOWN))
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_POWER_ACTION_UNSUPPORTED);
    }

    this->power_action_ = PowerAction::POWER_ACTION_SHUTDOWN;
    this->start_next_phase();
    invocation.ret();
}

void SessionManager::RequestShutdown(MethodInvocation &invocation)
{
    KLOG_PROFILE("");

    this->Shutdown(invocation);
}

void SessionManager::CanShutdown(MethodInvocation &invocation)
{
    KLOG_PROFILE("");
    invocation.ret(this->power_.can_power_action(PowerAction::POWER_ACTION_SHUTDOWN));
}

void SessionManager::Logout(guint32 mode, MethodInvocation &invocation)
{
    KLOG_PROFILE("");

    if (this->current_phase_ > KSMPhase::KSM_PHASE_RUNNING)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_PHASE_INVALID);
    }

    if (!this->power_.can_power_action(PowerAction::POWER_ACTION_LOGOUT))
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_POWER_ACTION_UNSUPPORTED);
    }

    this->power_action_ = PowerAction::POWER_ACTION_LOGOUT;
    this->start_next_phase();
    invocation.ret();
}

void SessionManager::CanLogout(MethodInvocation &invocation)
{
    KLOG_PROFILE("");
    invocation.ret(true);
}

void SessionManager::Reboot(MethodInvocation &invocation)
{
    KLOG_PROFILE("");

    if (this->current_phase_ > KSMPhase::KSM_PHASE_RUNNING)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_PHASE_INVALID);
    }

    if (!this->power_.can_power_action(PowerAction::POWER_ACTION_REBOOT))
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_MANAGER_POWER_ACTION_UNSUPPORTED);
    }

    this->power_action_ = PowerAction::POWER_ACTION_REBOOT;
    this->start_next_phase();
    invocation.ret();
}

void SessionManager::RequestReboot(MethodInvocation &invocation)
{
    KLOG_PROFILE("");
    this->Reboot(invocation);
}

void SessionManager::CanReboot(MethodInvocation &invocation)
{
    KLOG_PROFILE("");
    invocation.ret(this->power_.can_power_action(PowerAction::POWER_ACTION_REBOOT));
}

void SessionManager::Setenv(const Glib::ustring &name, const Glib::ustring &value, MethodInvocation &invocation)
{
    KLOG_PROFILE("name: %s, value: %s.", name.c_str(), value.c_str());

    Utils::setenv(name, value);
    invocation.ret();
}

void SessionManager::init()
{
    KLOG_PROFILE("");

    this->power_.init();
    this->presence_->init();

    this->app_manager_->signal_app_exited().connect(sigc::mem_fun(this, &SessionManager::on_app_exited_cb));
    this->client_manager_->signal_client_added().connect(sigc::mem_fun(this, &SessionManager::on_client_added_cb));
    this->client_manager_->signal_client_deleted().connect(sigc::mem_fun(this, &SessionManager::on_client_deleted_cb));
    this->client_manager_->signal_interact_request().connect(sigc::mem_fun(this, &SessionManager::on_interact_request_cb));
    this->client_manager_->signal_interact_done().connect(sigc::mem_fun(this, &SessionManager::on_interact_done_cb));
    this->client_manager_->signal_shutdown_canceled().connect(sigc::mem_fun(this, &SessionManager::on_shutdown_canceled_cb));
    this->client_manager_->signal_end_session_phase2_request().connect(sigc::mem_fun(this, &SessionManager::on_end_session_phase2_request_cb));
    this->client_manager_->signal_end_session_response().connect(sigc::mem_fun(this, &SessionManager::on_end_session_response_cb));

    this->inhibitor_manager_->signal_inhibitor_added().connect(sigc::mem_fun(this, &SessionManager::on_inhibitor_added_cb));
    this->inhibitor_manager_->signal_inhibitor_deleted().connect(sigc::mem_fun(this, &SessionManager::on_inhibitor_deleted_cb));

    this->dbus_connect_id_ = Gio::DBus::own_name(Gio::DBus::BUS_TYPE_SESSION,
                                                 KSM_DBUS_NAME,
                                                 sigc::mem_fun(this, &SessionManager::on_bus_acquired),
                                                 sigc::mem_fun(this, &SessionManager::on_name_acquired),
                                                 sigc::mem_fun(this, &SessionManager::on_name_lost));
}

void SessionManager::process_phase()
{
    auto phase_str = Utils::phase_enum2str(this->current_phase_);
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

void SessionManager::process_phase_startup()
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
            this->waiting_apps_timeout_id_ = timeout.connect_seconds(sigc::mem_fun(this, &SessionManager::on_phase_startup_timeout),
                                                                     KSM_PHASE_STARTUP_TIMEOUT);
        }
    }
    else
    {
        this->start_next_phase();
    }
}

void SessionManager::process_phase_query_end_session()
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
        this->waiting_clients_timeout_id_ = timeout.connect(sigc::bind(sigc::mem_fun(this, &SessionManager::on_waiting_session_timeout),
                                                                       sigc::mem_fun(this, &SessionManager::query_end_session_complete)),
                                                            300);
    }
    else
    {
        this->start_next_phase();
    }
}

void SessionManager::query_end_session_complete()
{
    KLOG_PROFILE("");

    if (this->current_phase_ != KSMPhase::KSM_PHASE_QUERY_END_SESSION)
    {
        KLOG_WARNING("The phase is error. phase: %s.", Utils::phase_enum2str(this->current_phase_).c_str());
        return;
    }

    this->waiting_clients_.clear();
    this->waiting_clients_timeout_id_.disconnect();

    if (this->exit_query_window_)
    {
        this->exit_query_window_->present();
        return;
    }

    this->exit_query_window_ = ExitQueryWindow::create(this->power_action_);
    this->exit_query_window_->signal_response().connect(sigc::mem_fun(this, &SessionManager::on_exit_window_response));

    this->exit_query_window_->show_all();
}

void SessionManager::cancel_end_session()
{
    KLOG_PROFILE("");

    RETURN_IF_TRUE(this->current_phase_ < KSMPhase::KSM_PHASE_QUERY_END_SESSION);

    // 如果取消退出，则之前通过Interact Request请求加入的抑制器需要全部移除
    this->inhibitor_manager_->delete_inhibitors_with_startup_id();

    for (auto client : this->client_manager_->get_clients())
    {
        if (!client->cancel_end_session())
        {
            KLOG_WARNING("Failed to cancel end session for the client: %s.", client->get_id().c_str());
        }
    }

    this->exit_query_window_ = nullptr;
    this->power_action_ = PowerAction::POWER_ACTION_NONE;

    // 回到运行阶段
    this->current_phase_ = KSMPhase::KSM_PHASE_RUNNING;
    // 不能在process_phase中清理第二阶段缓存客户端，因为在调用process_phase_end_session_phase2函数时需要使用
    this->phase2_request_clients_.clear();
    this->process_phase();
}

void SessionManager::process_phase_end_session_phase1()
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
        this->waiting_clients_timeout_id_ = timeout.connect_seconds(sigc::bind(sigc::mem_fun(this, &SessionManager::on_waiting_session_timeout),
                                                                               sigc::mem_fun(this, &SessionManager::start_next_phase)),
                                                                    30);
    }
    else
    {
        this->start_next_phase();
    }
}

void SessionManager::process_phase_end_session_phase2()
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
        this->waiting_clients_timeout_id_ = timeout.connect_seconds(sigc::bind(sigc::mem_fun(this, &SessionManager::on_waiting_session_timeout),
                                                                               sigc::mem_fun(this, &SessionManager::start_next_phase)),
                                                                    30);
    }
    else
    {
        this->start_next_phase();
    }
}

void SessionManager::process_phase_exit()
{
    for (auto client : this->client_manager_->get_clients())
    {
        if (!client->stop())
        {
            KLOG_WARNING("Failed to stop client: %s.", client->get_id().c_str());
        }
    }

    this->maybe_restart_user_bus();
    this->start_next_phase();
}

void SessionManager::maybe_restart_user_bus()
{
    RETURN_IF_TRUE(!SystemdLogin1::get_default()->is_last_session());

    KLOG_DEBUG("Restart dbus.service.");

    try
    {
        auto systemd_proxy = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BUS_TYPE_SESSION,
                                                                   SYSTEMD_DBUS_NAME,
                                                                   SYSTEMD_DBUS_OBJECT_PATH,
                                                                   SYSTEMD_DBUS_INTERFACE_NAME);

        auto g_parameters = g_variant_new("(ss)", "dbus.service", "replace");
        Glib::VariantContainerBase parameters(g_parameters, false);
        systemd_proxy->call_sync("TryRestartUnit", parameters);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("%s", e.what().c_str());
    }
}

void SessionManager::start_next_phase()
{
    bool start_next_phase = true;

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
        start_next_phase = false;
        break;
    default:
        break;
    }

    if (start_next_phase)
    {
        this->current_phase_ = (KSMPhase)(this->current_phase_ + 1);
        auto phase_str = Utils::phase_enum2str(this->current_phase_);
        KLOG_DEBUG("Start next phase: %s.", phase_str.c_str());
        this->process_phase();
    }
    else
    {
        auto phase_str = Utils::phase_enum2str(this->current_phase_);
        KLOG_DEBUG("Keep current phase: %s.", phase_str.c_str());
    }
}

void SessionManager::quit_session()
{
    KLOG_DEBUG("Quit Session.");

    this->power_.do_power_action(this->power_action_);
}

void SessionManager::on_app_startup_finished(std::shared_ptr<App> app)
{
    RETURN_IF_FALSE(app);

    auto iter = std::remove_if(this->waiting_apps_.begin(),
                               this->waiting_apps_.end(),
                               [app](std::shared_ptr<App> item) -> bool
                               {
                                   return item->get_app_id() == app->get_app_id();
                               });

    this->waiting_apps_.erase(iter, this->waiting_apps_.end());

    if (this->waiting_apps_.size() == 0 &&
        this->current_phase_ < KSMPhase::KSM_PHASE_APPLICATION)
    {
        if (this->waiting_apps_timeout_id_)
        {
            this->waiting_apps_timeout_id_.disconnect();
        }
        this->start_next_phase();
    }
}

bool SessionManager::on_phase_startup_timeout()
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

bool SessionManager::on_waiting_session_timeout(std::function<void(void)> phase_complete_callback)
{
    KLOG_PROFILE("");

    for (auto client : this->waiting_clients_)
    {
        KLOG_WARNING("The client %s doesn't response message. phase: %s",
                     client->get_id().c_str(),
                     Utils::phase_enum2str(this->current_phase_).c_str());

        // 将交互超时的客户端加入抑制器
        this->inhibitor_manager_->add_inhibitor(client->get_app_id(),
                                                0,
                                                _("This program isn't responding"),
                                                KSMInhibitorFlag::KSM_INHIBITOR_FLAG_QUIT,
                                                client->get_id());
    }

    phase_complete_callback();
    return false;
}

void SessionManager::on_exit_window_response(int32_t response_id)
{
    KLOG_PROFILE("response id: %d.", response_id);

    this->exit_query_window_ = nullptr;

    switch (response_id)
    {
    case ExitQueryResponse::EXIT_QUERY_RESPONSE_CANCEL:
        this->cancel_end_session();
        break;
    case ExitQueryResponse::EXIT_QUERY_RESPONSE_OK:
        this->start_next_phase();
        break;
    default:
        break;
    }
}

void SessionManager::on_app_exited_cb(std::shared_ptr<App> app)
{
    this->on_app_startup_finished(app);
}

void SessionManager::on_client_added_cb(std::shared_ptr<Client> client)
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

    // 此时说明kiran-session-daemon已经关闭掉与mate-settings-daemon冲突的插件，这时可以运行mate-settings-daemon
    auto iter = std::find_if(this->waiting_apps_.begin(), this->waiting_apps_.end(), [](std::shared_ptr<App> app)
                             { return app->get_app_id() == "mate-settings-daemon.desktop"; });
    if (app->get_app_id() == "kiran-session-daemon.desktop" &&
        iter != this->waiting_apps_.end())
    {
        KLOG_DEBUG("Start to boot %s.", (*iter)->get_app_id().c_str());
        (*iter)->start();
    }

    this->on_app_startup_finished(app);
}

void SessionManager::on_client_deleted_cb(std::shared_ptr<Client> client)
{
    RETURN_IF_FALSE(client);
    KLOG_DEBUG("client: %s.", client->get_id().c_str());
    // 客户端断开连接或者异常退出后无法再响应会话管理的请求，因此这里主动调用一次客户端响应回调函数来处理该客户端
    this->on_end_session_response_cb(client);

    if (this->current_phase_ == KSMPhase::KSM_PHASE_RUNNING)
    {
        auto app = this->app_manager_->get_app_by_startup_id(client->get_id());
        if (app && app->get_auto_restart())
        {
            auto idle = Glib::MainContext::get_default()->signal_idle();
            idle.connect_once([app]()
                              { app->restart(); });
        }
    }
}

void SessionManager::on_interact_request_cb(std::shared_ptr<Client> client)
{
    RETURN_IF_FALSE(this->current_phase_ == KSMPhase::KSM_PHASE_QUERY_END_SESSION);

    // 需要跟用户进行交互，因此此处需要添加抑制器，不能马上进行退出操作

    this->inhibitor_manager_->add_inhibitor(client->get_app_id(),
                                            0,
                                            _("This program is blocking exit."),
                                            KSMInhibitorFlag::KSM_INHIBITOR_FLAG_QUIT,
                                            client->get_id());

    /* 当通过SmsSaveYourself向xsmp客户端发起QueryEndSession时，客户端可能要求与用户交互(InteractRequest)，也可能直接回复SaveYourselfDone，
       无论是那种情况，都表示客户端已经收到了会话管理发送的消息，因此应该将该客户端从等待队列中删除。这里为了方便直接调用on_end_session_response_cb
       函数进行处理了，如果后续需要针对这两个消息做不同的处理，此处的逻辑需要修改。*/
    this->on_end_session_response_cb(client);
}

void SessionManager::on_interact_done_cb(std::shared_ptr<Client> client)
{
    this->inhibitor_manager_->delete_inhibitor_by_startup_id(client->get_id());
}

void SessionManager::on_shutdown_canceled_cb(std::shared_ptr<Client> client)
{
    KLOG_PROFILE("client: %s.", client->get_id().c_str());

    this->cancel_end_session();
}

void SessionManager::on_end_session_phase2_request_cb(std::shared_ptr<Client> client)
{
    KLOG_PROFILE("client: %s.", client->get_id().c_str());

    // 需要第一阶段结束的所有客户端响应后才能进入第二阶段，因此这里先缓存第二阶段请求的客户端
    this->phase2_request_clients_.push_back(client);
}

void SessionManager::on_end_session_response_cb(std::shared_ptr<Client> client)
{
    KLOG_PROFILE("current phase: %s",
                 Utils::phase_enum2str(this->current_phase_).c_str());

    RETURN_IF_FALSE(client);
    RETURN_IF_TRUE(this->current_phase_ < KSMPhase::KSM_PHASE_QUERY_END_SESSION);

    auto iter = std::remove_if(this->waiting_clients_.begin(),
                               this->waiting_clients_.end(),
                               [client](std::shared_ptr<Client> item)
                               {
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
            KLOG_WARNING("The phase is invalid. current phase: %s, client id: %s.",
                         Utils::phase_enum2str(this->current_phase_).c_str(),
                         client->get_id().c_str());
            break;
        }
    }
}

void SessionManager::on_inhibitor_added_cb(std::shared_ptr<Inhibitor> inhibitor)
{
    this->InhibitorAdded_signal.emit(inhibitor->cookie);
}

void SessionManager::on_inhibitor_deleted_cb(std::shared_ptr<Inhibitor> inhibitor)
{
    this->InhibitorRemoved_signal.emit(inhibitor->cookie);
}

void SessionManager::on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name)
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

void SessionManager::on_name_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name)
{
    KLOG_DEBUG("Success to register dbus name: %s", name.c_str());
}

void SessionManager::on_name_lost(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name)
{
    KLOG_WARNING("Lost dbus name: %s", name.c_str());
}
}  // namespace Daemon
}  // namespace Kiran