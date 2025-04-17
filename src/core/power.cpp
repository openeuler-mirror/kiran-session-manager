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
#include <QGSettings>
#include "lib/base/base.h"
#include "lib/dbus/display-manager.h"
#include "lib/dbus/screensaver.h"
#include "lib/dbus/systemd-login1.h"

namespace Kiran
{
Power::Power(QObject *parent) : QObject(parent)
{
    m_settings = new QGSettings(KSM_SCHEMA_ID, "", this);
}

void Power::init()
{
}

bool Power::canPowerAction(PowerAction powerAction)
{
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
    KLOG_INFO() << "Do power action" << powerActionEnum2Str(powerAction);

    switch (powerAction)
    {
    case PowerAction::POWER_ACTION_SWITCH_USER:
        return switchUser();
    case PowerAction::POWER_ACTION_LOGOUT:
        break;
    case PowerAction::POWER_ACTION_SUSPEND:
        return suspend();
    case PowerAction::POWER_ACTION_HIBERNATE:
        return hibernate();
    case PowerAction::POWER_ACTION_SHUTDOWN:
        return shutdown();
    case PowerAction::POWER_ACTION_REBOOT:
        return reboot();
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
    uint32_t throttle = 0;

    RETURN_VAL_IF_TRUE(!SystemdLogin1::getDefault()->canSuspend(), false);

    // 挂起之前判断是否锁定屏幕
    auto lockscreen = m_settings->get(KSM_SCHEMA_KEY_SCREEN_LOCKED_WHEN_SUSPEND).toBool();
    if (lockscreen)
    {
        throttle = ScreenSaver::getDefault()->lockAndThrottle("suspend");
    }

    auto retval = SystemdLogin1::getDefault()->suspend();

    ScreenSaver::getDefault()->poke();
    if (throttle)
    {
        ScreenSaver::getDefault()->removeThrottle(throttle);
    }

    return retval;
}

bool Power::hibernate()
{
    uint32_t throttle = 0;

    RETURN_VAL_IF_TRUE(!SystemdLogin1::getDefault()->canHibernate(), false);

    // 休眠之前判断是否锁定屏幕
    auto lockscreen = m_settings->get(KSM_SCHEMA_KEY_SCREEN_LOCKED_WHEN_HIBERNATE).toBool();
    if (lockscreen)
    {
        throttle = ScreenSaver::getDefault()->lockAndThrottle("hibernate");
    }

    auto retval = SystemdLogin1::getDefault()->hibernate();

    ScreenSaver::getDefault()->poke();
    if (throttle)
    {
        ScreenSaver::getDefault()->removeThrottle(throttle);
    }

    return retval;
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

QString Power::powerActionEnum2Str(PowerAction powerAction)
{
    switch (powerAction)
    {
    case PowerAction::POWER_ACTION_SWITCH_USER:
        return "switch user";
    case PowerAction::POWER_ACTION_LOGOUT:
        return "logout";
    case PowerAction::POWER_ACTION_SUSPEND:
        return "suspend";
    case PowerAction::POWER_ACTION_HIBERNATE:
        return "hibernate";
    case PowerAction::POWER_ACTION_SHUTDOWN:
        return "shutdown";
    case PowerAction::POWER_ACTION_REBOOT:
        return "reboot";
    default:
        break;
    }
    return "unknown";
}

}  // namespace Kiran