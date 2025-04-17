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

#include <QObject>

class IdleMonitorAdaptor;

namespace Kiran
{
class IdleMonitor : public QObject
{
    Q_OBJECT

public:
    IdleMonitor(QObject *parent = nullptr);
    virtual ~IdleMonitor(){};

    void init();

public Q_SLOTS:  // METHODS
    int AddIdleTimeout(qulonglong interval);
    qulonglong GetIdletime();
    void RemoveIdleTimeout(int id);
    void SimulateUserActivity();

Q_SIGNALS:  // SIGNALS
    void ResumingFromIdle();
    void TimeoutReached(int id, qulonglong interval);

private:
    IdleMonitorAdaptor *m_dbusAdaptor;

    QString m_clientID;
};

}  // namespace Kiran
