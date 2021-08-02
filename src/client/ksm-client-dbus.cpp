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

#include "src/client/ksm-client-dbus.h"

namespace Kiran
{
#define KSM_CLIENT_DBUS_OBJECT_PATH "/org/gnome/SessionManager/Client"
int32_t KSMClientDBus::client_count_ = 0;

KSMClientDBus::KSMClientDBus(const std::string &startup_id) : KSMClient(startup_id)
{
    Gio::DBus::Connection::get(Gio::DBus::BUS_TYPE_SESSION, sigc::mem_fun(this, &KSMClientDBus::on_bus_acquired));
    this->object_path_ = fmt::format("{0}{1}", KSM_CLIENT_DBUS_OBJECT_PATH, ++KSMClientDBus::client_count_);
}

bool KSMClientDBus::cancel_end_session()
{
    this->CancelEndSession_signal.emit(false);
    return true;
}

bool KSMClientDBus::query_end_session(bool interact)
{
    this->QueryEndSession_signal.emit(0);
    return true;
}

bool KSMClientDBus::end_session(bool save_data)
{
    this->EndSession_signal.emit(0);
    return true;
}

bool KSMClientDBus::end_session_phase2()
{
    // dbus不存在第二阶段
    return true;
}

bool KSMClientDBus::stop()
{
    return true;
}

// void KSMClientDBus::GetAppId(ClientStub::MethodInvocation &invocation)
// {
//     KLOG_PROFILE("");
//     invocation.ret(Glib::ustring());
// }

// void KSMClientDBus::GetStartupId(ClientStub::MethodInvocation &invocation)
// {
//     KLOG_PROFILE("");
//     invocation.ret(this->get_id());
// }

// void KSMClientDBus::GetRestartStyleHint(ClientStub::MethodInvocation &invocation)
// {
//     KLOG_PROFILE("");
//     invocation.ret(0);
// }

// void KSMClientDBus::GetUnixProcessId(ClientStub::MethodInvocation &invocation)
// {
//     // TODO:
// }

// void KSMClientDBus::GetStatus(ClientStub::MethodInvocation &invocation)
// {
//     KLOG_PROFILE("");
//     invocation.ret(0);
// }

// void KSMClientDBus::Stop(ClientStub::MethodInvocation &invocation)
// {
//     KLOG_PROFILE("");
//     this->stop();
//     invocation.ret();
// }

void KSMClientDBus::EndSessionResponse(bool is_ok,
                                       const Glib::ustring &reason,
                                       ClientPrivateStub::MethodInvocation &invocation)
{
    KLOG_PROFILE("");

    this->end_session_response_.emit(is_ok);
    invocation.ret();
}

void KSMClientDBus::on_bus_acquired(Glib::RefPtr<Gio::AsyncResult> &result)
{
    auto connection = Gio::DBus::Connection::get_finish(result);
    if (!connection)
    {
        KLOG_WARNING("Failed to connect dbus.");
        return;
    }

    try
    {
        // this->ClientStub::register_object(connection, this->get_object_path());
        this->ClientPrivateStub::register_object(connection, this->get_object_path());
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("Register object_path %s fail: %s.", this->get_object_path().c_str(), e.what().c_str());
    }
}

}  // namespace Kiran
