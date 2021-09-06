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

#pragma once

// #include <client_dbus_stub.h>
#include <client_private_dbus_stub.h>
#include "src/client/client.h"

namespace Kiran
{
namespace Daemon
{
class ClientDBus : public Client, public gnome::SessionManager::ClientPrivateStub
{
public:
    ClientDBus(const std::string &startup_id, const std::string &dbus_name);
    virtual ~ClientDBus();

    ClientType get_type() { return ClientType::CLIENT_TYPE_DBUS; };
    Glib::DBusObjectPathString get_object_path() { return this->object_path_; };
    std::string get_dbus_name() { return this->dbus_name_; };

    virtual bool cancel_end_session();
    virtual bool query_end_session(bool interact);
    virtual bool end_session(bool save_data);
    virtual bool end_session_phase2();
    virtual bool stop();

    //
    sigc::signal<void, bool> signal_end_session_response() { return this->end_session_response_; };

protected:
    // virtual void GetAppId(ClientStub::MethodInvocation &invocation);
    // virtual void GetStartupId(ClientStub::MethodInvocation &invocation);
    // virtual void GetRestartStyleHint(ClientStub::MethodInvocation &invocation);
    // virtual void GetUnixProcessId(ClientStub::MethodInvocation &invocation);
    // virtual void GetStatus(ClientStub::MethodInvocation &invocation);
    // virtual void Stop(ClientStub::MethodInvocation &invocation);

    virtual void EndSessionResponse(bool is_ok, const Glib::ustring &reason, ClientPrivateStub::MethodInvocation &invocation);

private:
    void on_bus_acquired(Glib::RefPtr<Gio::AsyncResult> &result);

private:
    std::string dbus_name_;
    sigc::signal<void, bool> end_session_response_;

    static int32_t client_count_;
    Glib::DBusObjectPathString object_path_;
};
}  // namespace Daemon
}  // namespace Kiran
