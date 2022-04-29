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

#include "src/wrapper/display-manager.h"

namespace Kiran
{
namespace Daemon
{
#define DISPLAY_MANAGER_DBUS_NAME "org.freedesktop.DisplayManager"
#define DISPLAY_MANAGER_DBUS_INTERFACE "org.freedesktop.DisplayManager.Seat"

DisplayManager::DisplayManager()
{
    auto xdg_seat_object_path = Glib::getenv("XDG_SEAT_PATH");

    try
    {
        if (xdg_seat_object_path.empty())
        {
            KLOG_WARNING("Not found XDG_SEAT_PATH.");
        }
        else
        {
            KLOG_DEBUG("XDG_SEAT_PATH: %s.", xdg_seat_object_path.c_str());

            this->display_proxy_ = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BUS_TYPE_SYSTEM,
                                                                         DISPLAY_MANAGER_DBUS_NAME,
                                                                         xdg_seat_object_path,
                                                                         DISPLAY_MANAGER_DBUS_INTERFACE,
                                                                         Glib::RefPtr<Gio::DBus::InterfaceInfo>(),
                                                                         Gio::DBus::PROXY_FLAGS_DO_NOT_AUTO_START);
        }
    }
    catch (const Glib::Error& e)
    {
        KLOG_WARNING("%s", e.what().c_str());
    }
}

std::shared_ptr<DisplayManager> DisplayManager::instance_ = nullptr;
std::shared_ptr<DisplayManager> DisplayManager::get_default()
{
    if (!instance_)
    {
        instance_ = std::make_shared<DisplayManager>();
    }
    return instance_;
}

bool DisplayManager::can_switch_user()
{
    RETURN_VAL_IF_FALSE(this->display_proxy_, false);

    try
    {
        Glib::VariantBase property;
        this->display_proxy_->get_cached_property(property, "CanSwitch");
        RETURN_VAL_IF_TRUE(property.gobj() == NULL, false);
        auto can_switch = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(property).get();
        return can_switch;
    }
    catch (const std::exception& e)
    {
        KLOG_WARNING("%s", e.what());
        return false;
    }
}

bool DisplayManager::switch_user()
{
    if (this->display_proxy_)
    {
        try
        {
            this->display_proxy_->call_sync("SwitchToGreeter", Glib::VariantContainerBase());
        }
        catch (const Glib::Error& e)
        {
            KLOG_WARNING("%s", e.what().c_str());
            return false;
        }
    }
    else
    {
        KLOG_WARNING("Unable to start LightDM greeter.");
        return false;
    }
    return true;
}
}  // namespace Daemon
}  // namespace Kiran
