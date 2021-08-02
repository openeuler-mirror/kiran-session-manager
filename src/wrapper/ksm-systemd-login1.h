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

#include "lib/base.h"

namespace Kiran
{
// 对systemd-login1的dbus接口的封装
class KSMSystemdLogin1
{
public:
    KSMSystemdLogin1();
    virtual ~KSMSystemdLogin1(){};

    void init();

    // 是否支持关机
    bool can_shutdown() { return this->can_do_method("CanPowerOff"); };
    // 是否支持重启
    bool can_reboot() { return this->can_do_method("CanReboot"); };
    // 是否支持休眠
    bool can_hibernate() { return this->can_do_method("CanHibernate"); };
    // 是否支持挂起
    bool can_suspend() { return this->can_do_method("CanSuspend"); };
    // 关机
    bool shutdown() { return this->do_method("PowerOff"); };
    // 重启
    bool reboot() { return this->do_method("Reboot"); };
    // 休眠
    bool hibernate() { return this->do_method("Hibernate"); };
    // 挂起
    bool suspend() { return this->do_method("Suspend"); };

private:
    bool can_do_method(const std::string &method_name);
    bool do_method(const std::string &method_name);

private:
    // systemd-login1
    Glib::RefPtr<Gio::DBus::Proxy> login1_proxy_;
};
}  // namespace Kiran