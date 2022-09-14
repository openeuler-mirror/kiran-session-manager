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

#include <QDBusContext>

class QGSettings;
class PresenceAdaptor;
class IdleMonitorProxy;
class QDBusServiceWatcher;

namespace Kiran
{
class Presence : public QObject,
                 protected QDBusContext
{
    Q_OBJECT

    Q_PROPERTY(uint status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QString statusText READ statusText WRITE setStatusText NOTIFY statusTextChanged)
public:
    Presence(QObject *parent);
    virtual ~Presence(){};

    void init();

    // 空闲超时是否开启，如果添加了抑制器则不应该触发空闲超时信号
    void enableIdleTimeout(bool enabled);

    uint status() { return this->m_status; }
    void setStatus(uint status);

    QString statusText() { return this->m_statusText; }
    void setStatusText(const QString &statusText);

public Q_SLOTS:  // METHODS
    void SetStatus(uint status);
    void SetStatusText(const QString &status_text);
Q_SIGNALS:  // SIGNALS
    void statusChanged();
    void statusTextChanged();
    void StatusChanged(uint status);
    void StatusTextChanged(const QString &status_text);

private:
    void updateIdleXlarm();

private Q_SLOTS:
    void onSettingsChanged(const QString &key);
    void onNameAcquired(const QString &dbusName);
    void onNameLost(const QString &dbusName);
    void onResumingFromIdle();
    void onTimeoutReached(int id, qulonglong interval);

private:
    QGSettings *m_settings;
    IdleMonitorProxy *m_idleMonitorProxy;
    QDBusServiceWatcher *m_serviceWatcher;
    // 空闲超时被开启
    bool m_enabledIdleTimeout;
    // 空闲超时时间（分钟）
    int32_t m_idleTimeout;
    int32_t m_idleTimeoutIdentifier;

    uint m_status;
    QString m_statusText;

    PresenceAdaptor *m_dbusAdaptor;
};
}  // namespace Kiran
