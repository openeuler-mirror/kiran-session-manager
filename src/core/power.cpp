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

#include "src/core/power.h"
#include "lib/base/base.h"
#include "lib/dbus/display-manager.h"
#include "lib/dbus/screensaver.h"
#include "lib/dbus/systemd-login1.h"

namespace Kiran
{
Power::Power(QObject *parent) : QObject(parent)
{
}

void Power::init()
{
}

bool Power::canPowerAction(PowerAction powerAction)
{
    KLOG_DEBUG() << "Power action: " << powerAction;

    switch (powerAction)
    {
    case PowerAction::POWER_ACTION_SWITCH_USER:
        return DisplayManager::getDefault()->canSwitchUser();
    case PowerAction::POWER_ACTION_LOGOUT:
        return true;
    case PowerAction::POWER_ACTION_SUSPEND:
        return SystemdLogin1::getDefault()->canSuspend();
    case PowerAction::POWER_ACTION_HIBERNATE:
        return SystemdLogin1::getDefault()->canHibernate();
    case PowerAction::POWER_ACTION_SHUTDOWN:
        return SystemdLogin1::getDefault()->canShutdown();
    case PowerAction::POWER_ACTION_REBOOT:
        return SystemdLogin1::getDefault()->canReboot();
    default:
        break;
    }
    return true;
}

bool Power::doPowerAction(PowerAction powerAction)
{
    KLOG_DEBUG() << "Power action: " << powerAction;

    switch (powerAction)
    {
    case PowerAction::POWER_ACTION_SWITCH_USER:
        return this->switchUser();
    case PowerAction::POWER_ACTION_LOGOUT:
        break;
    case PowerAction::POWER_ACTION_SUSPEND:
        return this->suspend();
    case PowerAction::POWER_ACTION_HIBERNATE:
        return this->hibernate();
    case PowerAction::POWER_ACTION_SHUTDOWN:
        return this->shutdown();
    case PowerAction::POWER_ACTION_REBOOT:
        return this->reboot();
    default:
        break;
    }
    return true;
}

bool Power::switchUser()
{
    RETURN_VAL_IF_TRUE(!DisplayManager::getDefault()->canSwitchUser(), false);
    return DisplayManager::getDefault()->switchUser();
}

bool Power::suspend()
{
    RETURN_VAL_IF_TRUE(!SystemdLogin1::getDefault()->canSuspend(), false);

    // 这里忽略锁屏失败的情况
    ScreenSaver::getDefault()->lock();
    return SystemdLogin1::getDefault()->suspend();
}

bool Power::hibernate()
{
    RETURN_VAL_IF_TRUE(!SystemdLogin1::getDefault()->canHibernate(), false);

    // 这里忽略锁屏失败的情况
    ScreenSaver::getDefault()->lock();
    return SystemdLogin1::getDefault()->hibernate();
}

bool Power::shutdown()
{
    RETURN_VAL_IF_TRUE(!SystemdLogin1::getDefault()->canShutdown(), false);
    return SystemdLogin1::getDefault()->shutdown();
}

bool Power::reboot()
{
    RETURN_VAL_IF_TRUE(!SystemdLogin1::getDefault()->canReboot(), false);
    return SystemdLogin1::getDefault()->reboot();
}

}  // namespace Kiran