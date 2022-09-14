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

#include "src/idle/idle-monitor.h"
#include <KIdleTime>
#include <QDBusConnection>
#include <QGuiApplication>
#include "lib/base/base.h"
#include "src/idle/idle_monitor_adaptor.h"

namespace Kiran
{
IdleMonitor::IdleMonitor(QObject* parent) : QObject(parent)
{
    this->m_dbusAdaptor = new IdleMonitorAdaptor(this);
}

void IdleMonitor::init()
{
    connect(KIdleTime::instance(), &KIdleTime::resumingFromIdle, [this]()
            { Q_EMIT this->ResumingFromIdle(); });
    connect(KIdleTime::instance(), static_cast<void (KIdleTime::*)(int, int)>(&KIdleTime::timeoutReached), [this](int identifier, int msec)
            {
                Q_EMIT this->TimeoutReached(identifier, msec);
                KIdleTime::instance()->catchNextResumeEvent();
            });

    KIdleTime::instance()->catchNextResumeEvent();

    auto sessionConnection = QDBusConnection::sessionBus();

    if (!sessionConnection.registerService(KSM_IDLE_DBUS_NAME))
    {
        KLOG_WARNING() << "Failed to register dbus name: " << KSM_IDLE_DBUS_NAME;
    }

    if (!sessionConnection.registerObject(KSM_IDLE_DBUS_OBJECT_PATH, this))
    {
        KLOG_WARNING() << "Can't register object:" << sessionConnection.lastError();
    }
}

int IdleMonitor::AddIdleTimeout(qulonglong interval)
{
    return KIdleTime::instance()->addIdleTimeout(interval);
}

qulonglong IdleMonitor::GetIdletime()
{
    return KIdleTime::instance()->idleTime();
}

void IdleMonitor::RemoveIdleTimeout(int id)
{
    KIdleTime::instance()->removeIdleTimeout(id);
}

void IdleMonitor::SimulateUserActivity()
{
    KIdleTime::instance()->simulateUserActivity();
}

}  // namespace Kiran
