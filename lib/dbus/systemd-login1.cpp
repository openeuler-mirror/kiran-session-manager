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

#include "lib/dbus/systemd-login1.h"
#include <qt5-log-i.h>
#include <unistd.h>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QSharedPointer>
#include "lib/base/def.h"

struct SessionItem
{
    QString sessionID;
    uint userID;
    QString userName;
    QString seatID;
    QDBusObjectPath objectPath;
};

Q_DECLARE_METATYPE(SessionItem);

QDBusArgument &operator<<(QDBusArgument &argument, const SessionItem &item)
{
    argument.beginStructure();
    argument << item.sessionID << item.userID << item.userName << item.seatID << item.objectPath;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, SessionItem &item)
{
    argument.beginStructure();
    argument >> item.sessionID >> item.userID >> item.userName >> item.seatID >> item.objectPath;
    argument.endStructure();
    return argument;
}

namespace Kiran
{
#define LOGIN1_DBUS_NAME "org.freedesktop.login1"
#define LOGIN1_DBUS_OBJECT_PATH "/org/freedesktop/login1"
#define LOGIN1_MANAGER_DBUS_INTERFACE "org.freedesktop.login1.Manager"
#define LOGIN1_SESSION_DBUS_INTERFACE "org.freedesktop.login1.Session"
/**
 * 执行重启的过程中，会先退出会话，如果是当前账户的最后一个会话，会重启会话中的dbus服务，以杀死会话下的所有进程，退出会话
 * 然后调用Logind的CanReboot，再调用Reboot实现重启
 * 在调用CanReboot的过程中，如果等待dbus消息超时的时间为300ms，等待时间太短，如果超时则不会继续执行Reboot，从而导致重启功能失效
 * 因此，将等待超时时间改为10000ms
*/
#define LOGIN1_DBUS_METHOD_TIMEOUT_MS 10000

SystemdLogin1::SystemdLogin1()
{
    auto sendMessage = QDBusMessage::createMethodCall(LOGIN1_DBUS_NAME,
                                                      LOGIN1_DBUS_OBJECT_PATH,
                                                      LOGIN1_MANAGER_DBUS_INTERFACE,
                                                      "GetSessionByPID");
    sendMessage << (uint32_t)getpid();

    auto replyMessage = QDBusConnection::systemBus().call(sendMessage, QDBus::Block, DBUS_TIMEOUT_MS);

    if (replyMessage.type() == QDBusMessage::ErrorMessage)
    {
        KLOG_WARNING() << "Call GetSessionByPID failed: " << replyMessage.errorMessage();
        return;
    }

    this->m_sessionObjectPath = replyMessage.arguments().takeFirst().value<QDBusObjectPath>();

    qDBusRegisterMetaType<SessionItem>();
}

QSharedPointer<SystemdLogin1> SystemdLogin1::m_instance = nullptr;
QSharedPointer<SystemdLogin1> SystemdLogin1::getDefault()
{
    if (!m_instance)
    {
        m_instance = QSharedPointer<SystemdLogin1>::create();
        m_instance->init();
    }
    return m_instance;
}

bool SystemdLogin1::setIdleHint(bool isIdle)
{
    auto sendMessage = QDBusMessage::createMethodCall(LOGIN1_DBUS_NAME,
                                                      this->m_sessionObjectPath.path(),
                                                      LOGIN1_SESSION_DBUS_INTERFACE,
                                                      "SetIdleHint");
    sendMessage << isIdle;

    auto replyMessage = QDBusConnection::systemBus().call(sendMessage, QDBus::Block, DBUS_TIMEOUT_MS);

    if (replyMessage.type() == QDBusMessage::ErrorMessage)
    {
        KLOG_WARNING() << "Call SetIdleHint failed: " << replyMessage.errorMessage();
        return false;
    }
    return true;
}

bool SystemdLogin1::isLastSession()
{
    auto sendMessage = QDBusMessage::createMethodCall(LOGIN1_DBUS_NAME,
                                                      LOGIN1_DBUS_OBJECT_PATH,
                                                      LOGIN1_MANAGER_DBUS_INTERFACE,
                                                      "ListSessions");

    auto replyMessage = QDBusConnection::systemBus().call(sendMessage, QDBus::Block, DBUS_TIMEOUT_MS);
    if (replyMessage.type() == QDBusMessage::ErrorMessage)
    {
        KLOG_WARNING() << "Call ListSessions failed: " << replyMessage.errorMessage();
        return false;
    }

    auto userUID = getuid();
    auto replyArgs = replyMessage.arguments();
    auto replySessions = replyArgs.takeFirst().value<QDBusArgument>();
    replySessions.beginArray();
    while (!replySessions.atEnd())
    {
        SessionItem sessionItem;
        replySessions >> sessionItem;

        CONTINUE_IF_TRUE(sessionItem.userID != userUID);
        CONTINUE_IF_TRUE(sessionItem.objectPath == this->m_sessionObjectPath);

        QString state;
        QString type;

#define GET_STRING_PROPERTY(propertyName, propertyVariable)                                                        \
    {                                                                                                              \
        auto propSendMessage = QDBusMessage::createMethodCall(LOGIN1_DBUS_NAME,                                    \
                                                              sessionItem.objectPath.path(),                       \
                                                              "org.freedesktop.DBus.Properties",                   \
                                                              "Get");                                              \
        propSendMessage << LOGIN1_SESSION_DBUS_INTERFACE << #propertyName;                                         \
        auto propReplyMessage = QDBusConnection::systemBus().call(propSendMessage, QDBus::Block, DBUS_TIMEOUT_MS); \
        if (propReplyMessage.type() == QDBusMessage::ErrorMessage)                                                 \
        {                                                                                                          \
            KLOG_WARNING() << "Get " << #propertyName << " property failed: " << propReplyMessage.errorMessage();  \
            continue;                                                                                              \
        }                                                                                                          \
        auto firstArg = propReplyMessage.arguments().takeFirst().value<QDBusVariant>();                            \
        propertyVariable = firstArg.variant().value<QString>();                                                    \
    }
        GET_STRING_PROPERTY(State, state)
        GET_STRING_PROPERTY(Type, type)

        CONTINUE_IF_TRUE(state == "closing");
        CONTINUE_IF_TRUE(type != "x11" && type != "wayland");

        KLOG_DEBUG() << "Has other session running: " << sessionItem.objectPath.path();
        return false;
    }
    return true;
}

void SystemdLogin1::init()
{
}

bool SystemdLogin1::canDoMethod(const QString &methodName)
{
    KLOG_DEBUG() << "Call function " << methodName;

    auto sendMessage = QDBusMessage::createMethodCall(LOGIN1_DBUS_NAME,
                                                      LOGIN1_DBUS_OBJECT_PATH,
                                                      LOGIN1_MANAGER_DBUS_INTERFACE,
                                                      methodName);

    auto replyMessage = QDBusConnection::systemBus().call(sendMessage, QDBus::Block, LOGIN1_DBUS_METHOD_TIMEOUT_MS);
    if (replyMessage.type() == QDBusMessage::ErrorMessage)
    {
        KLOG_WARNING() << "Call " << methodName << " failed: " << replyMessage.errorMessage();
        return false;
    }

    auto canResult = replyMessage.arguments().takeFirst().toString();

    KLOG_DEBUG() << "Function " << methodName << " return " << canResult;
    // TODO: 需要确认challenge状态是否可以执行电源动作
    return (canResult == "yes" || canResult == "challenge");
}

bool SystemdLogin1::doMethod(const QString &methodName)
{
    KLOG_DEBUG() << "Call function " << methodName;

    auto sendMessage = QDBusMessage::createMethodCall(LOGIN1_DBUS_NAME,
                                                      LOGIN1_DBUS_OBJECT_PATH,
                                                      LOGIN1_MANAGER_DBUS_INTERFACE,
                                                      methodName);

    sendMessage << false;
    auto replyMessage = QDBusConnection::systemBus().call(sendMessage, QDBus::Block, LOGIN1_DBUS_METHOD_TIMEOUT_MS);
    if (replyMessage.type() == QDBusMessage::ErrorMessage)
    {
        KLOG_WARNING() << "Call " << methodName << " failed: " << replyMessage.errorMessage();
        return false;
    }
    return true;
}
}  // namespace Kiran