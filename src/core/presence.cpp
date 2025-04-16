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

#include "src/core/presence.h"
#include <QDBusServiceWatcher>
#include <QGSettings>
#include "ksm-i.h"
#include "lib/base/base.h"
#include "src/core/idle_monitor_proxy.h"
#include "src/core/presence_adaptor.h"

namespace Kiran
{
Presence::Presence(QObject *parent) : QObject(parent),
                                      m_idleMonitorProxy(nullptr),
                                      m_enabledIdleTimeout(true),
                                      m_idleTimeout(0),
                                      m_idleTimeoutIdentifier(0),
                                      m_status(KSMPresenceStatus::KSM_PRESENCE_STATUS_AVAILABLE)
{
    this->m_settings = new QGSettings(KSM_SCHEMA_ID, "", this);
    this->m_dbusAdaptor = new PresenceAdaptor(this);
    this->m_serviceWatcher = new QDBusServiceWatcher(this);
}

void Presence::init()
{
    this->m_idleTimeout = this->m_settings->get(KSM_SCHEMA_KEY_IDLE_DELAY).toInt();

    this->updateIdleXlarm();

    if (!connect(this->m_settings, SIGNAL(changed(const QString &)), this, SLOT(onSettingsChanged(const QString &))))
    {
        KLOG_WARNING() << "Can't connect session manager settings changed signal";
    }

    auto sessionConnection = QDBusConnection::sessionBus();
    if (!sessionConnection.registerObject(KSM_PRESENCE_DBUS_OBJECT_PATH, this))
    {
        KLOG_WARNING() << "Can't register object:" << sessionConnection.lastError();
    }

    this->m_serviceWatcher->setConnection(sessionConnection);
    this->m_serviceWatcher->setWatchedServices(QStringList(KSM_IDLE_DBUS_NAME));
    this->m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration);

    connect(this->m_serviceWatcher, SIGNAL(serviceRegistered(const QString &)), this, SLOT(onNameAcquired(const QString &)));
    connect(this->m_serviceWatcher, SIGNAL(serviceUnregistered(const QString &)), this, SLOT(onNameLost(const QString &)));
}

void Presence::enableIdleTimeout(bool enabled)
{
    KLOG_DEBUG() << "Enabled/Disable idle time: " << enabled;

    RETURN_IF_TRUE(this->m_enabledIdleTimeout == enabled);
    this->m_enabledIdleTimeout = enabled;
    this->updateIdleXlarm();
}

void Presence::setStatus(uint status)
{
    RETURN_IF_TRUE(this->m_status == status);
    this->m_status = status;
    Q_EMIT this->statusChanged();
    Q_EMIT this->StatusChanged(this->m_status);
}

void Presence::setStatusText(const QString &statusText)
{
    RETURN_IF_TRUE(this->m_statusText == statusText);

    this->m_statusText = statusText;
    Q_EMIT this->statusTextChanged();
    Q_EMIT this->StatusTextChanged(this->m_statusText);
}

void Presence::SetStatus(uint status)
{
    if (status >= KSM_PRESENCE_STATUS_LAST)
    {
        DBUS_ERROR_REPLY_AND_RET(QDBusError::InvalidArgs, KSMErrorCode::ERROR_PRESENCE_STATUS_INVALID);
    }
    this->setStatus(status);
}

void Presence::SetStatusText(const QString &status_text)
{
    this->setStatusText(status_text);
}

void Presence::updateIdleXlarm()
{
    KLOG_DEBUG() << "Enabled: " << this->m_enabledIdleTimeout << ", idle timeout: " << this->m_idleTimeout;

    if (!this->m_idleMonitorProxy)
    {
        KLOG_DEBUG() << "The idle monitor proxy hasn't exist.";
        return;
    }

    if (this->m_idleTimeoutIdentifier > 0)
    {
        auto reply = this->m_idleMonitorProxy->RemoveIdleTimeout(this->m_idleTimeoutIdentifier);
        reply.waitForFinished();
        this->m_idleTimeoutIdentifier = 0;
    }

    if (this->m_idleTimeout > 0)
    {
        auto reply = this->m_idleMonitorProxy->AddIdleTimeout(this->m_idleTimeout * 60000);
        reply.waitForFinished();
        if (reply.isError())
        {
            KLOG_WARNING() << "Failed to add idle time: " << reply.error().message();
        }
        else
        {
            this->m_idleTimeoutIdentifier = reply.value();
        }
    }
}

void Presence::onSettingsChanged(const QString &key)
{
    if (key == KSM_SCHEMA_KEY_IDLE_DELAY)
    {
        auto idleTimeout = this->m_settings->get(key).toInt();
        if (idleTimeout != this->m_idleTimeout)
        {
            this->m_idleTimeout = idleTimeout;
            this->updateIdleXlarm();
        }
    }
}

void Presence::onNameAcquired(const QString &dbusName)
{
    KLOG_DEBUG() << "Receive name acquired: " << dbusName;

    this->m_idleMonitorProxy = new IdleMonitorProxy(KSM_IDLE_DBUS_NAME,
                                                    KSM_IDLE_DBUS_OBJECT_PATH,
                                                    QDBusConnection::sessionBus(),
                                                    this);

    connect(this->m_idleMonitorProxy, SIGNAL(ResumingFromIdle()), this, SLOT(onResumingFromIdle()));
    connect(this->m_idleMonitorProxy, SIGNAL(TimeoutReached(int, qulonglong)), this, SLOT(onTimeoutReached(int, qulonglong)));
    this->updateIdleXlarm();
}

void Presence::onNameLost(const QString &dbusName)
{
    KLOG_DEBUG() << "Receive name lost: " << dbusName;

    if (this->m_idleMonitorProxy)
    {
        delete this->m_idleMonitorProxy;
        this->m_idleMonitorProxy = nullptr;
    }
}

void Presence::onResumingFromIdle()
{
    KLOG_DEBUG() << "Receive alarm signal.";
    this->setStatus(KSMPresenceStatus::KSM_PRESENCE_STATUS_AVAILABLE);
}

void Presence::onTimeoutReached(int id, qulonglong interval)
{
    KLOG_DEBUG() << "Receive alarm triggered signal.";

    if (id == this->m_idleTimeoutIdentifier)
    {
        this->setStatus(KSMPresenceStatus::KSM_PRESENCE_STATUS_IDLE);
    }
}

}  // namespace Kiran
