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
#include "src/wrapper/ksm-display-manager.h"
#include "src/wrapper/ksm-screensaver.h"
#include "src/wrapper/ksm-systemd-login1.h"

namespace Kiran
{
enum KSMPowerAction
{
    KSM_POWER_ACTION_NONE,
    // 切换用户
    KSM_POWER_ACTION_SWITCH_USER,
    // 注销
    KSM_POWER_ACTION_LOGOUT,
    // 挂起
    KSM_POWER_ACTION_SUSPEND,
    // 休眠
    KSM_POWER_ACTION_HIBERNATE,
    // 关机
    KSM_POWER_ACTION_SHUTDOWN,
    // 重启
    KSM_POWER_ACTION_REBOOT,
};

class KSMPower
{
public:
    KSMPower();
    virtual ~KSMPower(){};

    void init();

    std::shared_ptr<KSMSystemdLogin1> get_systemd_login1() { return this->systemd_login1_; };

    // 是否可以执行该电源行为
    bool can_power_action(KSMPowerAction power_action);
    // 执行电源行为
    bool do_power_action(KSMPowerAction power_action);

private:
    // 切换用户
    bool switch_user();
    // 挂起
    bool suspend();
    // 休眠
    bool hibernate();
    // 关机
    bool shutdown();
    // 重启
    bool reboot();

private:
    std::shared_ptr<KSMSystemdLogin1> systemd_login1_;
    std::shared_ptr<KSMScreenSaver> screensaver_;
    std::shared_ptr<KSMDisplayManager> display_manager_;
};
}  // namespace Kiran