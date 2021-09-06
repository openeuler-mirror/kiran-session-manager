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
namespace Daemon
{
// 对screensaver的dbus接口的封装
class ScreenSaver
{
public:
    ScreenSaver();
    virtual ~ScreenSaver(){};

    void init();

    // 锁屏
    bool lock();

    // 添加throttle，禁止运行屏保主题。函数返回一个cookie，可以通过cookie移除该throttle
    uint32_t add_throttle(const std::string &reason);

    // 锁屏并添加throttle，如果失败则返回0，否则返回cookie
    uint32_t lock_and_throttle(const std::string &reason);

    // 移除throttle
    bool remove_throttle(uint32_t cookie);

    // 模拟用户激活操作
    bool poke();

private:
    Glib::RefPtr<Gio::DBus::Proxy> screensaver_proxy_;
};
}  // namespace Daemon
}  // namespace Kiran