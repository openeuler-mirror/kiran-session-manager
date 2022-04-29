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

#include "src/wrapper/systemd-login1.h"

namespace Kiran
{
namespace Daemon
{
#define LOGIN1_DBUS_NAME "org.freedesktop.login1"
#define LOGIN1_DBUS_OBJECT_PATH "/org/freedesktop/login1"
#define LOGIN1_MANAGER_DBUS_INTERFACE "org.freedesktop.login1.Manager"
#define LOGIN1_SESSION_DBUS_INTERFACE "org.freedesktop.login1.Session"

SystemdLogin1::SystemdLogin1()
{
    try
    {
        this->login1_proxy_ = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BUS_TYPE_SYSTEM,
                                                                    LOGIN1_DBUS_NAME,
                                                                    LOGIN1_DBUS_OBJECT_PATH,
                                                                    LOGIN1_MANAGER_DBUS_INTERFACE);

        auto g_parameters = g_variant_new("(u)", (uint32_t)getpid());
        Glib::VariantContainerBase parameters(g_parameters, false);

        auto retval = this->login1_proxy_->call_sync("GetSessionByPID", parameters);
        auto v1 = retval.get_child(0);
        this->session_object_path_ = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::DBusObjectPathString>>(v1).get();

        this->session_proxy_ = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BUS_TYPE_SYSTEM,
                                                                     LOGIN1_DBUS_NAME,
                                                                     this->session_object_path_,
                                                                     LOGIN1_SESSION_DBUS_INTERFACE);
    }
    catch (const Glib::Error& e)
    {
        KLOG_WARNING("%s", e.what().c_str());
    }
}

std::shared_ptr<SystemdLogin1> SystemdLogin1::instance_ = nullptr;
std::shared_ptr<SystemdLogin1> SystemdLogin1::get_default()
{
    if (!instance_)
    {
        instance_ = std::make_shared<SystemdLogin1>();
        instance_->init();
    }
    return instance_;
}

bool SystemdLogin1::set_idle_hint(bool is_idle)
{
    KLOG_PROFILE("");

    RETURN_VAL_IF_FALSE(this->session_proxy_, false);

    try
    {
        auto g_parameters = g_variant_new("(b)", is_idle);
        Glib::VariantContainerBase parameters(g_parameters, false);
        this->session_proxy_->call_sync("SetIdleHint", parameters);
    }
    catch (const Glib::Error& e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return false;
    }
    return true;
}

bool SystemdLogin1::is_last_session()
{
    KLOG_PROFILE("");

    try
    {
        auto user_uid = getuid();
        auto retval = this->login1_proxy_->call_sync("ListSessions");
        auto out_param1 = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(retval.get_child(0));

        using SessionTupleType = std::tuple<Glib::ustring, uint32_t, Glib::ustring, Glib::ustring, Glib::DBusObjectPathString>;
        for (uint32_t i = 0; i < out_param1.get_n_children(); ++i)
        {
            auto param1_item = Glib::VariantBase::cast_dynamic<Glib::VariantContainerBase>(out_param1.get_child(i));
            // auto param1_item2 = Glib::VariantBase::cast_dynamic<Glib::Variant<uint32_t>>(param1_item.get_child(1)).get();
            // auto param1_item5 = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::DBusObjectPathString>>(param1_item.get_child(4)).get();
            auto session_tuple = Glib::VariantBase::cast_dynamic<Glib::Variant<SessionTupleType>>(param1_item).get();

            CONTINUE_IF_TRUE(std::get<1>(session_tuple) != user_uid);
            CONTINUE_IF_TRUE(std::get<4>(session_tuple) == this->session_object_path_);
            // CONTINUE_IF_TRUE(param1_item2 != user_uid);
            // CONTINUE_IF_TRUE(param1_item5 == this->session_object_path_);

            Glib::ustring state;
            Glib::ustring type;

            try
            {
                auto session_proxy = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BUS_TYPE_SYSTEM,
                                                                           LOGIN1_DBUS_NAME,
                                                                           //    param1_item5,
                                                                           std::get<4>(session_tuple),
                                                                           LOGIN1_SESSION_DBUS_INTERFACE);

                Glib::VariantBase variant_state;
                Glib::VariantBase type_state;
                session_proxy->get_cached_property(variant_state, "State");
                session_proxy->get_cached_property(type_state, "Type");

                state = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(variant_state).get();
                type = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(type_state).get();
            }
            catch (const Glib::Error& e)
            {
                KLOG_WARNING("%s", e.what().c_str());
                continue;
            }
            catch (const std::exception& e)
            {
                KLOG_WARNING("%s", e.what());
                continue;
            }

            CONTINUE_IF_TRUE(state == "closing");
            CONTINUE_IF_TRUE(type != "x11" && type != "wayland");

            KLOG_DEBUG("Has other session running: %s.", std::get<4>(session_tuple).c_str());
            return false;
        }
    }
    catch (const Glib::Error& e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return false;
    }
    catch (const std::exception& e)
    {
        KLOG_WARNING("%s", e.what());
        return false;
    }
    return true;
}

void SystemdLogin1::init()
{
}

bool SystemdLogin1::can_do_method(const std::string& method_name)
{
    KLOG_PROFILE("method name: %s.", method_name.c_str());

    RETURN_VAL_IF_FALSE(this->login1_proxy_, false);

    try
    {
        auto retval = this->login1_proxy_->call_sync(method_name, Glib::VariantContainerBase());
        auto v1 = retval.get_child(0);
        auto v1_str = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(v1).get();
        KLOG_DEBUG("Call %s result: %s.", method_name.c_str(), v1_str.c_str());
        return (v1_str == "yes" || v1_str == "challenge");
    }
    catch (const Glib::Error& e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return false;
    }
    catch (const std::exception& e)
    {
        KLOG_WARNING("%s", e.what());
        return false;
    }
    return true;
}

bool SystemdLogin1::do_method(const std::string& method_name)
{
    KLOG_PROFILE("method name: %s.", method_name.c_str());

    RETURN_VAL_IF_FALSE(this->login1_proxy_, false);

    auto g_parameters = g_variant_new("(b)", FALSE);
    Glib::VariantContainerBase parameters(g_parameters, false);

    try
    {
        this->login1_proxy_->call_sync(method_name, parameters);
    }
    catch (const Glib::Error& e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return false;
    }
    return true;
}
}  // namespace Daemon
}  // namespace Kiran
