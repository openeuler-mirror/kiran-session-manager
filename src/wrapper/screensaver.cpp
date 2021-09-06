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

#include "src/wrapper/screensaver.h"

namespace Kiran
{
namespace Daemon
{
#define SCREENSAVER_DBUS_NAME "org.mate.ScreenSaver"
#define SCREENSAVER_DBUS_OBJECT_PATH "/org/mate/ScreenSaver"
#define SCREENSAVER_DBUS_INTERFACE "org.mate.ScreenSaver"

// 屏保锁屏后，检查锁屏状态的最大次数
#define SCREENSAVER_LOCK_CHECK_MAX_COUNT 50

ScreenSaver::ScreenSaver()
{
}

void ScreenSaver::init()
{
    try
    {
        this->screensaver_proxy_ = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BUS_TYPE_SESSION,
                                                                         SCREENSAVER_DBUS_NAME,
                                                                         SCREENSAVER_DBUS_OBJECT_PATH,
                                                                         SCREENSAVER_DBUS_INTERFACE);
    }
    catch (const Glib::Error& e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return;
    }
}

bool ScreenSaver::lock()
{
    KLOG_PROFILE("");
    RETURN_VAL_IF_FALSE(this->screensaver_proxy_, false);

    /* 注意：Lock函数无返回值，因此这里锁屏有可能会失败，如果需要检查锁屏是否成功，需要调用
       GetActive函数检查屏保活动状态。*/
    try
    {
        this->screensaver_proxy_->call("Lock", Gio::SlotAsyncReady(), Glib::VariantContainerBase());
    }
    catch (const Glib::Error& e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return false;
    }

    return true;
}

uint32_t ScreenSaver::add_throttle(const std::string& reason)
{
    KLOG_PROFILE("reason: %s.", reason.c_str());
    RETURN_VAL_IF_FALSE(this->screensaver_proxy_, 0);

    auto parameters = g_variant_new("(ss)", "Power screensaver", reason.c_str());
    Glib::VariantContainerBase base(parameters, false);
    Glib::VariantContainerBase retval;

    try
    {
        retval = this->screensaver_proxy_->call_sync("Throttle", base);
    }
    catch (const Glib::Error& e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return 0;
    }

    try
    {
        auto v1 = retval.get_child(0);
        auto cookie = Glib::VariantBase::cast_dynamic<Glib::Variant<uint32_t>>(v1).get();
        KLOG_DEBUG("cookie: %u.", cookie);
        return cookie;
    }
    catch (const std::exception& e)
    {
        KLOG_WARNING("%s", e.what());
    }
    return 0;
}

uint32_t ScreenSaver::lock_and_throttle(const std::string& reason)
{
    KLOG_PROFILE("reason: %s.", reason.c_str());

    RETURN_VAL_IF_FALSE(this->lock(), 0);
    return this->add_throttle(reason);
}

bool ScreenSaver::remove_throttle(uint32_t cookie)
{
    KLOG_PROFILE("cookie: %u", cookie);
    RETURN_VAL_IF_FALSE(this->screensaver_proxy_, false);

    auto parameters = g_variant_new("(u)", cookie);
    Glib::VariantContainerBase base(parameters, false);

    try
    {
        this->screensaver_proxy_->call_sync("UnThrottle", base);
    }
    catch (const Glib::Error& e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return false;
    }

    return true;
}

bool ScreenSaver::poke()
{
    KLOG_PROFILE("");
    RETURN_VAL_IF_FALSE(this->screensaver_proxy_, false);

    try
    {
        this->screensaver_proxy_->call("SimulateUserActivity", Gio::SlotAsyncReady(), Glib::VariantContainerBase());
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