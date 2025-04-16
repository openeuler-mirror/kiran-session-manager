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

#include "src/core/client/client-dbus.h"
#include "lib/base/base.h"
#include "src/core/client_dbus_adaptor.h"

namespace Kiran
{
#define KSM_CLIENT_DBUS_OBJECT_PATH "/org/gnome/SessionManager/Client"
#define KSM_CLIENT_DBUS_INTERFACE "org.gnome.SessionManager.ClientPrivate"

int32_t ClientDBus::m_clientCount = 0;

ClientDBus::ClientDBus(const QString &startupID,
                       const QString &dbusName,
                       const QString &appID,
                       QObject *parent) : Client(startupID, parent),
                                          m_dbusName(dbusName),
                                          m_appID(appID)
{
    this->m_objectPath = QDBusObjectPath(QString("%1/%2").arg(KSM_CLIENT_DBUS_OBJECT_PATH).arg(++ClientDBus::m_clientCount));
    this->m_dbusAdaptor = new ClientDBusAdaptor(this);

    if (!this->m_appID.endsWith(".desktop"))
    {
        this->m_appID += ".desktop";
    }

    auto sessionConnection = QDBusConnection::sessionBus();

    if (!sessionConnection.registerObject(this->m_objectPath.path(), KSM_CLIENT_DBUS_INTERFACE, this))
    {
        KLOG_ERROR() << "Can't register object:" << sessionConnection.lastError();
    }
}

ClientDBus::~ClientDBus()
{
    KLOG_DEBUG() << "client " << this->getID() << " is destroyed.";
}

QString ClientDBus::getAppID()
{
    RETURN_VAL_IF_TRUE(this->m_appID.length() > 0, this->m_appID);
    return this->Client::getAppID();
}

bool ClientDBus::cancelEndSession()
{
    Q_EMIT this->CancelEndSession(false);
    return true;
}

bool ClientDBus::queryEndSession(bool interact)
{
    Q_EMIT this->QueryEndSession(0);
    return true;
}

bool ClientDBus::endSession(bool save_data)
{
    Q_EMIT this->EndSession(0);
    return true;
}

bool ClientDBus::endSessionPhase2()
{
    // dbus不存在第二阶段
    return true;
}

bool ClientDBus::stop()
{
    return true;
}

void ClientDBus::EndSessionResponse(bool is_ok, const QString &reason)
{
    Q_EMIT this->endSessionResponse(is_ok);
}

}  // namespace Kiran
