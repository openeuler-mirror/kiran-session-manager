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

#include "lib/dbus/display-manager.h"
#include <qt5-log-i.h>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QSharedPointer>
#include "lib/base/def.h"

namespace Kiran
{
#define DISPLAY_MANAGER_DBUS_NAME "org.freedesktop.DisplayManager"
#define DISPLAY_MANAGER_DBUS_INTERFACE "org.freedesktop.DisplayManager.Seat"

DisplayManager::DisplayManager()
{
    this->m_xdgSeatPath = qgetenv("XDG_SEAT_PATH");

    if (this->m_xdgSeatPath.isEmpty())
    {
        KLOG_WARNING() << "Not found XDG_SEAT_PATH";
    }
}

QSharedPointer<DisplayManager> DisplayManager::m_instance = nullptr;
QSharedPointer<DisplayManager> DisplayManager::getDefault()
{
    if (!m_instance)
    {
        m_instance = QSharedPointer<DisplayManager>::create();
    }
    return m_instance;
}

bool DisplayManager::canSwitchUser()
{
    QDBusMessage sendMessage = QDBusMessage::createMethodCall(DISPLAY_MANAGER_DBUS_NAME,
                                                              this->m_xdgSeatPath,
                                                              "org.freedesktop.DBus.Properties",
                                                              "Get");
    sendMessage << QString(DISPLAY_MANAGER_DBUS_INTERFACE) << QString("CanSwitch");
    QDBusMessage replyMessage = QDBusConnection::systemBus().call(sendMessage, QDBus::Block, DBUS_TIMEOUT_MS);

    if (replyMessage.type() == QDBusMessage::ErrorMessage)
    {
        KLOG_WARNING() << "Call Get failed: " << replyMessage.errorMessage();
        return false;
    }

    QList<QVariant> argList = replyMessage.arguments();
    if (argList.size() == 0)
    {
        KLOG_WARNING() << "no arguments";
        return false;
    }

    QVariant firstArg = argList.takeFirst();
    QDBusVariant busVariant = firstArg.value<QDBusVariant>();
    QVariant iconFileVar = busVariant.variant();
    return iconFileVar.toBool();
}

bool DisplayManager::switchUser()
{
    auto sendMessage = QDBusMessage::createMethodCall(DISPLAY_MANAGER_DBUS_NAME,
                                                      this->m_xdgSeatPath,
                                                      DISPLAY_MANAGER_DBUS_INTERFACE,
                                                      "SwitchToGreeter");

    auto replyMessage = QDBusConnection::systemBus().call(sendMessage, QDBus::Block, DBUS_TIMEOUT_MS);

    if (replyMessage.type() == QDBusMessage::ErrorMessage)
    {
        KLOG_WARNING() << "Call SwitchToGreeter failed: " << replyMessage.errorMessage();
        return false;
    }
    return true;
}
}  // namespace Kiran
