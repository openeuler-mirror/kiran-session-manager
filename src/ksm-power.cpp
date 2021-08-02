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

#include "src/ksm-power.h"
#include <gtkmm.h>

namespace Kiran
{
#define SCREENSAVER_DBUS_NAME "org.mate.ScreenSaver"
#define SCREENSAVER_DBUS_OBJECT_PATH "/org/mate/ScreenSaver"
#define SCREENSAVER_DBUS_INTERFACE "org.mate.ScreenSaver"

KSMPower::KSMPower()
{
    this->display_manager_ = std::make_shared<KSMDisplayManager>();
    this->systemd_login1_ = std::make_shared<KSMSystemdLogin1>();
    this->screensaver_ = std::make_shared<KSMScreenSaver>();
}

void KSMPower::init()
{
}

bool KSMPower::can_power_action(KSMPowerAction power_action)
{
    KLOG_PROFILE("power action: %d.", int32_t(power_action));

    switch (power_action)
    {
    case KSMPowerAction::KSM_POWER_ACTION_SWITCH_USER:
        return this->display_manager_->can_switch_user();
    case KSMPowerAction::KSM_POWER_ACTION_LOGOUT:
        return true;
    case KSMPowerAction::KSM_POWER_ACTION_SUSPEND:
        return this->systemd_login1_->can_suspend();
    case KSMPowerAction::KSM_POWER_ACTION_HIBERNATE:
        return this->systemd_login1_->can_hibernate();
    case KSMPowerAction::KSM_POWER_ACTION_SHUTDOWN:
        return this->systemd_login1_->can_shutdown();
    case KSMPowerAction::KSM_POWER_ACTION_REBOOT:
        return this->systemd_login1_->can_reboot();
    default:
        break;
    }
    return true;
}

bool KSMPower::do_power_action(KSMPowerAction power_action)
{
    KLOG_PROFILE("power action: %d.", int32_t(power_action));

    switch (power_action)
    {
    case KSMPowerAction::KSM_POWER_ACTION_SWITCH_USER:
        return this->switch_user();
    case KSMPowerAction::KSM_POWER_ACTION_LOGOUT:
        gtk_main_quit();
        break;
    case KSMPowerAction::KSM_POWER_ACTION_SUSPEND:
        return this->suspend();
    case KSMPowerAction::KSM_POWER_ACTION_HIBERNATE:
        return this->hibernate();
    case KSMPowerAction::KSM_POWER_ACTION_SHUTDOWN:
        return this->shutdown();
    case KSMPowerAction::KSM_POWER_ACTION_REBOOT:
        return this->reboot();
    default:
        break;
    }
    return true;
}

bool KSMPower::switch_user()
{
    RETURN_VAL_IF_TRUE(!this->display_manager_->can_switch_user(), false);
    return this->display_manager_->switch_user();
}

bool KSMPower::suspend()
{
    RETURN_VAL_IF_TRUE(!this->systemd_login1_->can_suspend(), false);

    // 这里忽略锁屏失败的情况
    this->screensaver_->lock();
    return this->systemd_login1_->suspend();
}

bool KSMPower::hibernate()
{
    RETURN_VAL_IF_TRUE(!this->systemd_login1_->can_hibernate(), false);

    // 这里忽略锁屏失败的情况
    this->screensaver_->lock();
    return this->systemd_login1_->hibernate();
}

bool KSMPower::shutdown()
{
    RETURN_VAL_IF_TRUE(!this->systemd_login1_->can_shutdown(), false);
    return this->systemd_login1_->shutdown();
}

bool KSMPower::reboot()
{
    RETURN_VAL_IF_TRUE(!this->systemd_login1_->reboot(), false);
    return this->systemd_login1_->reboot();
}

}  // namespace Kiran