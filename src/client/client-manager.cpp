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

#include "src/client/client-manager.h"
#include "src/client/client-dbus.h"
#include "src/client/client-xsmp.h"
#include "src/utils.h"

#include <X11/ICE/ICEconn.h>
#include <X11/ICE/ICElib.h>
#include <X11/ICE/ICEutil.h>

namespace Kiran
{
namespace Daemon
{
ClientManager::ClientManager(XsmpServer *xsmp_server) : xsmp_server_(xsmp_server)
{
    try
    {
        this->dbus_daemon_proxy_ = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BUS_TYPE_SESSION,
                                                                         DAEMON_DBUS_NAME,
                                                                         DAEMON_DBUS_OBJECT_PATH,
                                                                         DAEMON_DBUS_INTERFACE_NAME);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("%s.", e.what().c_str());
    }
}

ClientManager *ClientManager::instance_ = nullptr;
void ClientManager::global_init(XsmpServer *xsmp_server)
{
    instance_ = new ClientManager(xsmp_server);
    instance_->init();
}

std::shared_ptr<ClientDBus> ClientManager::get_client_by_dbus_name(const std::string &dbus_name)
{
    for (auto iter : this->clients_)
    {
        auto client = iter.second;
        CONTINUE_IF_TRUE(client->get_type() != CLIENT_TYPE_DBUS);
        auto dbus_client = std::dynamic_pointer_cast<ClientDBus>(client);
        RETURN_VAL_IF_TRUE(dbus_client->get_dbus_name() == dbus_name, dbus_client);
    }
    return nullptr;
}

void ClientManager::init()
{
    this->dbus_daemon_proxy_->signal_signal().connect(sigc::mem_fun(this, &ClientManager::on_dbus_daemon_signal_cb));
    this->xsmp_server_->signal_new_client_connected().connect(sigc::mem_fun(this, &ClientManager::on_new_xsmp_client_connected_cb));
    this->xsmp_server_->signal_ice_conn_status_changed().connect(sigc::mem_fun(this, &ClientManager::on_ice_conn_status_changed_cb));
}

std::shared_ptr<ClientXsmp> ClientManager::add_client_xsmp(const std::string &startup_id,
                                                           SmsConn sms_conn)
{
    auto new_startup_id = startup_id;
    if (new_startup_id.empty())
    {
        new_startup_id = Utils::generate_startup_id();
    }

    auto client = std::make_shared<ClientXsmp>(new_startup_id, sms_conn);
    RETURN_VAL_IF_FALSE(this->add_client(client), nullptr);
    return client;
}

std::shared_ptr<ClientDBus> ClientManager::add_client_dbus(const std::string &startup_id, const std::string &dbus_name)
{
    KLOG_PROFILE("startup id: %s.", startup_id.c_str());

    auto new_startup_id = startup_id;
    if (new_startup_id.empty())
    {
        new_startup_id = Utils::generate_startup_id();
    }

    auto client = std::make_shared<ClientDBus>(new_startup_id, dbus_name);
    RETURN_VAL_IF_FALSE(this->add_client(client), nullptr);
    // 共享指针client不能作为bind被传递，如果这样做的话会导致共享指针无法被销毁（引用计数死循环）
    client->signal_end_session_response().connect(sigc::bind(sigc::mem_fun(this, &ClientManager::on_dbus_client_end_session_response),
                                                             client->get_id()));
    return client;
}

bool ClientManager::delete_client(const std::string &startup_id)
{
    auto client = this->get_client(startup_id);
    RETURN_VAL_IF_FALSE(client, false);

    this->clients_.erase(startup_id);
    this->client_deleted_.emit(client);
    return true;
}

bool ClientManager::add_client(std::shared_ptr<Client> client)
{
    RETURN_VAL_IF_FALSE(client, false);

    auto iter = this->clients_.emplace(client->get_id(), client);

    if (!iter.second)
    {
        KLOG_WARNING("The client %s already exist.", client->get_id());
        return false;
    }

    this->client_added_.emit(client);
    return true;
}

std::shared_ptr<ClientXsmp> ClientManager::get_client_by_sms_conn(SmsConn sms_conn)
{
    auto id = SmsClientID(sms_conn);
    auto client = this->get_client(POINTER_TO_STRING(id));

    if (!client)
    {
        KLOG_WARNING("The client isn't found. startup_id: %s.", POINTER_TO_STRING(id).c_str());
        return nullptr;
    }

    if (client->get_type() != ClientType::CLIENT_TYPE_XSMP)
    {
        KLOG_WARNING("The client type isn't xsmp. startup_id: %s.", client->get_id().c_str());
        return nullptr;
    }
    return std::dynamic_pointer_cast<ClientXsmp>(client);
}

std::shared_ptr<ClientXsmp> ClientManager::get_client_by_ice_conn(IceConn ice_conn)
{
    for (auto iter : this->clients_)
    {
        CONTINUE_IF_TRUE(iter.second->get_type() != ClientType::CLIENT_TYPE_XSMP);
        auto xsmp_client = std::dynamic_pointer_cast<ClientXsmp>(iter.second);

        if (SmsGetIceConnection(xsmp_client->get_sms_connection()) == ice_conn)
        {
            return xsmp_client;
        }
    }
    return nullptr;
}

void ClientManager::on_dbus_daemon_signal_cb(const Glib::ustring &sender_name,
                                             const Glib::ustring &signal_name,
                                             const Glib::VariantContainerBase &parameters)
{
    RETURN_IF_TRUE(signal_name != DAEMON_DBUS_SIGNAL_NAME_OWNER_CHANGED);

    if (!parameters.check_format_string("(sss)"))
    {
        KLOG_WARNING("The arguments format of signal NameOwnerChanged signal isn't (sss).");
        return;
    }

    Glib::VariantBase old_name_child;
    Glib::VariantBase new_name_child;
    parameters.get_child(old_name_child, 1);
    parameters.get_child(new_name_child, 2);

    auto old_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(old_name_child).get();
    auto new_name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(new_name_child).get();
    if (new_name.empty())
    {
        auto dbus_client = this->get_client_by_dbus_name(old_name);
        if (dbus_client)
        {
            KLOG_DEBUG("Remove client %s which dbus name is %s.", dbus_client->get_id().c_str(), old_name.c_str());
            this->delete_client(dbus_client->get_id());
        }
    }
}

void ClientManager::on_new_xsmp_client_connected_cb(unsigned long *mask_ret, SmsCallbacks *callbacks_ret)
{
    *mask_ret = 0;

    *mask_ret |= SmsRegisterClientProcMask;
    callbacks_ret->register_client.callback = &ClientManager::on_register_client_cb;
    callbacks_ret->register_client.manager_data = this;

    *mask_ret |= SmsInteractRequestProcMask;
    callbacks_ret->interact_request.callback = &ClientManager::on_interact_request_cb;
    callbacks_ret->interact_request.manager_data = this;

    *mask_ret |= SmsInteractDoneProcMask;
    callbacks_ret->interact_done.callback = &ClientManager::on_interact_done_cb;
    callbacks_ret->interact_done.manager_data = this;

    *mask_ret |= SmsSaveYourselfRequestProcMask;
    callbacks_ret->save_yourself_request.callback = &ClientManager::on_save_yourself_request_cb;
    callbacks_ret->save_yourself_request.manager_data = this;

    *mask_ret |= SmsSaveYourselfP2RequestProcMask;
    callbacks_ret->save_yourself_phase2_request.callback = &ClientManager::on_save_yourself_phase2_request_cb;
    callbacks_ret->save_yourself_phase2_request.manager_data = this;

    *mask_ret |= SmsSaveYourselfDoneProcMask;
    callbacks_ret->save_yourself_done.callback = &ClientManager::on_save_yourself_done_cb;
    callbacks_ret->save_yourself_done.manager_data = this;

    *mask_ret |= SmsCloseConnectionProcMask;
    callbacks_ret->close_connection.callback = &ClientManager::on_close_connection_cb;
    callbacks_ret->close_connection.manager_data = this;

    *mask_ret |= SmsSetPropertiesProcMask;
    callbacks_ret->set_properties.callback = &ClientManager::on_set_properties_cb;
    callbacks_ret->set_properties.manager_data = this;

    *mask_ret |= SmsDeletePropertiesProcMask;
    callbacks_ret->delete_properties.callback = &ClientManager::on_delete_properties_cb;
    callbacks_ret->delete_properties.manager_data = this;

    *mask_ret |= SmsGetPropertiesProcMask;
    callbacks_ret->get_properties.callback = &ClientManager::on_get_properties_cb;
    callbacks_ret->get_properties.manager_data = this;
}

void ClientManager::on_ice_conn_status_changed_cb(IceProcessMessagesStatus status, IceConn ice_conn)
{
    KLOG_PROFILE("");
    auto client = this->get_client_by_ice_conn(ice_conn);
    RETURN_IF_FALSE(client);

    switch (status)
    {
    case IceProcessMessagesIOError:
        KLOG_WARNING("The client %s Receive IceProcessMessagesIOError message. program name: %s",
                     client->get_id().c_str(),
                     client->get_program_name().c_str());
        this->delete_client(client->get_id());
        break;
    case IceProcessMessagesConnectionClosed:
        KLOG_DEBUG("The client %s Receive IceProcessMessagesConnectionClosed message. program name: %s.",
                   client->get_id().c_str(),
                   client->get_program_name().c_str());
        break;
    default:
        break;
    }
}

void ClientManager::on_dbus_client_end_session_response(bool is_ok, std::string startup_id)
{
    auto client = this->get_client(startup_id);
    RETURN_IF_FALSE(client);

    if (is_ok)
    {
        this->end_session_response_.emit(client);
    }
    else
    {
        KLOG_WARNING("The Client %s save failed.", client->get_id().c_str());
    }
}

Status ClientManager::on_register_client_cb(SmsConn sms_conn, SmPointer manager_data, char *previous_id)
{
    KLOG_PROFILE("");

    /* SMS协议规范：
       1. 如果应用程序第一次加入会话，则previouse_id为NULL，这时需要会话管理负责生成一个唯一的id，
       如果应用程序根据上一次的会话重启，则previouse_id记录的是上一次会话管理生成的id。
       2. 会话管理需要通过SmsRegisterClientReply函数告知客户端注册已经成功，
          2.1 如果previous_id不为NULL，则携带的client_id参数应该跟previous_id相同；
          2.2 如果previous_id为NULL，则使用会话管理生成的唯一id并需要向客户端发送"Save Yourself"消息，
          该消息携带的参数为type=Local, shutdown=False,interact-style=None, fast=False。*/

    auto client_manager = (ClientManager *)(manager_data);
    auto new_previous_id = POINTER_TO_STRING(previous_id);

    SCOPE_EXIT({
        if (previous_id)
        {
            free(previous_id);
        }
    });

    auto client = client_manager->add_client_xsmp(POINTER_TO_STRING(previous_id), sms_conn);
    RETURN_VAL_IF_FALSE(client, False);

    KLOG_DEBUG("startup id: %s, previous_id: %s.",
               client->get_id().c_str(),
               POINTER_TO_STRING(previous_id).c_str());

    g_autofree gchar *reply_id = g_strdup(client->get_id().c_str());
    SmsRegisterClientReply(sms_conn, reply_id);
    if (!previous_id)
    {
        SmsSaveYourself(sms_conn, SmSaveLocal, False, SmInteractStyleNone, False);
    }
    return True;
}

void ClientManager::on_interact_request_cb(SmsConn sms_conn, SmPointer manager_data, int dialog_type)
{
    KLOG_PROFILE("");

    auto client_manager = (ClientManager *)(manager_data);
    auto client = client_manager->get_client_by_sms_conn(sms_conn);
    RETURN_IF_FALSE(client);

    // TODO: 需要添加抑制器

    KLOG_DEBUG("startup id: %s, dialog type: %d.", client->get_id().c_str(), dialog_type);
    SmsInteract(sms_conn);
}

void ClientManager::on_interact_done_cb(SmsConn sms_conn, SmPointer manager_data, Bool cancel_shutdown)
{
    KLOG_PROFILE("");

    auto client_manager = (ClientManager *)(manager_data);
    auto client = client_manager->get_client_by_sms_conn(sms_conn);
    RETURN_IF_FALSE(client);

    KLOG_DEBUG("startup id: %s, cancel shutdown: %d.", client->get_id().c_str(), cancel_shutdown);
    if (cancel_shutdown)
    {
        client_manager->shutdown_canceled_.emit(client);
    }
}

void ClientManager::on_save_yourself_request_cb(SmsConn sms_conn,
                                                SmPointer manager_data,
                                                int save_type,
                                                Bool shutdown,
                                                int interact_style,
                                                Bool fast,
                                                Bool global)
{
    KLOG_PROFILE("");
    /* 如果global为False，则会话管理应该只向请求的客户端发送"Save Yourself"消息；如果global为True，
       则会话管理应该向所有客户端发送"Save Yourself"消息。当global为True时，我们只在shutdown为True
       的情况向所有客户端发送"Save Yourself"消息，对应的shutdown设置为True，其他情况忽略处理*/

    auto client_manager = (ClientManager *)(manager_data);
    auto client = client_manager->get_client_by_sms_conn(sms_conn);
    RETURN_IF_FALSE(client);

    KLOG_DEBUG("startup id: %s, save type: %d, shutdown: %d, interact style: %d, fast: %d, global: %d",
               client->get_id().c_str(),
               save_type,
               shutdown,
               interact_style,
               fast,
               global);

    if (shutdown && global)
    {
        client_manager->shutdown_request_.emit(client);
    }
    else if (!shutdown && !global)
    {
        SmsSaveYourself(sms_conn, SmSaveLocal, False, SmInteractStyleAny, False);
    }
}

void ClientManager::on_save_yourself_phase2_request_cb(SmsConn sms_conn, SmPointer manager_data)
{
    KLOG_PROFILE("");

    auto client_manager = (ClientManager *)(manager_data);
    auto client = client_manager->get_client_by_sms_conn(sms_conn);
    RETURN_IF_FALSE(client);
    client_manager->end_session_phase2_request_.emit(client);
}

void ClientManager::on_save_yourself_done_cb(SmsConn sms_conn, SmPointer manager_data, Bool success)
{
    KLOG_PROFILE("");

    auto client_manager = (ClientManager *)(manager_data);
    auto client = client_manager->get_client_by_sms_conn(sms_conn);
    RETURN_IF_FALSE(client);

    KLOG_DEBUG("startup id: %s, success: %d.", client->get_id().c_str(), success);

    SmsSaveComplete(sms_conn);
    client_manager->end_session_response_.emit(client);
}

void ClientManager::on_close_connection_cb(SmsConn sms_conn,
                                           SmPointer manager_data,
                                           int count,
                                           char **reason_msgs)
{
    KLOG_PROFILE("");

    auto client_manager = (ClientManager *)(manager_data);
    auto client = client_manager->get_client_by_sms_conn(sms_conn);
    RETURN_IF_FALSE(client);

    for (int32_t i = 0; i < count; i++)
    {
        KLOG_DEBUG("Client %s close reason: '%s'", client->get_id().c_str(), reason_msgs[i]);
    }
    SmFreeReasons(count, reason_msgs);
    client_manager->delete_client(client->get_id());
}

void ClientManager::on_set_properties_cb(SmsConn sms_conn,
                                         SmPointer manager_data,
                                         int num_props,
                                         SmProp **props)
{
    KLOG_PROFILE("");

    auto client_manager = (ClientManager *)(manager_data);
    auto client = client_manager->get_client_by_sms_conn(sms_conn);
    RETURN_IF_FALSE(client);

    for (int32_t i = 0; i < num_props; ++i)
    {
        client->update_property(props[i]);
    }
    free(props);
}

void ClientManager::on_delete_properties_cb(SmsConn sms_conn,
                                            SmPointer manager_data,
                                            int num_props,
                                            char **prop_names)
{
    KLOG_PROFILE("");

    auto client_manager = (ClientManager *)(manager_data);
    auto client = client_manager->get_client_by_sms_conn(sms_conn);
    RETURN_IF_FALSE(client);

    for (int32_t i = 0; i < num_props; ++i)
    {
        client->delete_property(prop_names[i]);
    }
    // 文档中没有提到需要释放该变量
    free(prop_names);
}

void ClientManager::on_get_properties_cb(SmsConn sms_conn, SmPointer manager_data)
{
    KLOG_PROFILE("");

    auto client_manager = (ClientManager *)(manager_data);
    auto client = client_manager->get_client_by_sms_conn(sms_conn);
    RETURN_IF_FALSE(client);

    auto props = client->get_properties();
    SmsReturnProperties(sms_conn, props->len, (SmProp **)props->pdata);
}
}  // namespace Daemon
}  // namespace Kiran
