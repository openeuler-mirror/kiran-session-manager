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
#include "src/core/client/client.h"

class ClientDBusAdaptor;

namespace Kiran
{
class ClientDBus : public Client
{
    Q_OBJECT
public:
    ClientDBus(const QString &startupID,
               const QString &dbusName,
               const QString &appID,
               QObject *parent);
    virtual ~ClientDBus();

    ClientType getType() { return ClientType::CLIENT_TYPE_DBUS; };
    QDBusObjectPath getObjectPath() { return this->m_objectPath; };
    QString getDBusName() { return this->m_dbusName; };

    virtual QString getAppID() override;
    virtual bool cancelEndSession() override;
    virtual bool queryEndSession(bool interact) override;
    virtual bool endSession(bool save_data) override;
    virtual bool endSessionPhase2() override;
    virtual bool stop() override;

Q_SIGNALS:
    void endSessionResponse(bool);

public Q_SLOTS:
    void EndSessionResponse(bool is_ok, const QString &reason);

Q_SIGNALS:
    void CancelEndSession(bool placeholder);
    void EndSession(uint flags);
    void QueryEndSession(uint flags);
    void Stop(bool placeholder);

private:
    QString m_dbusName;
    QString m_appID;

    static int32_t m_clientCount;
    QDBusObjectPath m_objectPath;

    ClientDBusAdaptor *m_dbusAdaptor;
};
}  // namespace Kiran
