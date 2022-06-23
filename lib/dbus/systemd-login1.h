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

#include <QDBusObjectPath>
#include <QSharedPointer>

namespace Kiran
{
// 对systemd-login1的dbus接口的封装
class SystemdLogin1
{
public:
    SystemdLogin1();
    virtual ~SystemdLogin1(){};

    static QSharedPointer<SystemdLogin1> getDefault();

    // 是否支持关机
    bool canShutdown() { return this->canDoMethod("CanPowerOff"); };
    // 是否支持重启
    bool canReboot() { return this->canDoMethod("CanReboot"); };
    // 是否支持休眠
    bool canHibernate() { return this->canDoMethod("CanHibernate"); };
    // 是否支持挂起
    bool canSuspend() { return this->canDoMethod("CanSuspend"); };
    // 关机
    bool shutdown() { return this->doMethod("PowerOff"); };
    // 重启
    bool reboot() { return this->doMethod("Reboot"); };
    // 休眠
    bool hibernate() { return this->doMethod("Hibernate"); };
    // 挂起
    bool suspend() { return this->doMethod("Suspend"); };
    // 设置空闲提示
    bool setIdleHint(bool isIdle);
    // 是否为当前用户的最后一个会话
    bool isLastSession();

private:
    void init();

    bool canDoMethod(const QString &methodName);
    bool doMethod(const QString &methodName);

private:
    static QSharedPointer<SystemdLogin1> m_instance;
    QDBusObjectPath m_sessionObjectPath;
};
}  // namespace Kiran
