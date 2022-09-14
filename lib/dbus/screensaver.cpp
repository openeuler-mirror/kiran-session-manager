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

#include "lib/dbus/screensaver.h"
#include <qt5-log-i.h>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QSharedPointer>
#include "lib/base/def.h"

namespace Kiran
{
#define SCREENSAVER_DBUS_NAME "com.kylinsec.Kiran.ScreenSaver"
#define SCREENSAVER_DBUS_OBJECT_PATH "/com/kylinsec/Kiran/ScreenSaver"
#define SCREENSAVER_DBUS_INTERFACE "com.kylinsec.Kiran.ScreenSaver"

// 屏保锁屏后，检查锁屏状态的最大次数
#define SCREENSAVER_LOCK_CHECK_MAX_COUNT 50

ScreenSaver::ScreenSaver()
{
}

QSharedPointer<ScreenSaver> ScreenSaver::m_instance = nullptr;
QSharedPointer<ScreenSaver> ScreenSaver::getDefault()
{
    if (!m_instance)
    {
        m_instance = QSharedPointer<ScreenSaver>::create();
        m_instance->init();
    }
    return m_instance;
}

bool ScreenSaver::lock()
{
    KLOG_DEBUG("Lock screen.");

    /* 注意：Lock函数无返回值，因此这里锁屏有可能会失败，如果需要检查锁屏是否成功，需要调用
       GetActive函数检查屏保活动状态。*/
    auto sendMessage = QDBusMessage::createMethodCall(SCREENSAVER_DBUS_NAME,
                                                      SCREENSAVER_DBUS_OBJECT_PATH,
                                                      SCREENSAVER_DBUS_INTERFACE,
                                                      "Lock");

    auto replyMessage = QDBusConnection::sessionBus().call(sendMessage, QDBus::NoBlock);

    if (replyMessage.type() == QDBusMessage::ErrorMessage)
    {
        KLOG_WARNING() << "Call Lock failed: " << replyMessage.errorMessage();
        return false;
    }
    return true;
}

uint32_t ScreenSaver::addThrottle(const QString& reason)
{
    KLOG_DEBUG() << "Add throttle, reason: " << reason;

    auto sendMessage = QDBusMessage::createMethodCall(SCREENSAVER_DBUS_NAME,
                                                      SCREENSAVER_DBUS_OBJECT_PATH,
                                                      SCREENSAVER_DBUS_INTERFACE,
                                                      "Throttle");
    sendMessage << QString("Power screensaver") << reason;

    auto replyMessage = QDBusConnection::sessionBus().call(sendMessage, QDBus::Block, DBUS_TIMEOUT_MS);

    if (replyMessage.type() == QDBusMessage::ErrorMessage)
    {
        KLOG_WARNING() << "Call Throttle failed: " << replyMessage.errorMessage();
        return 0;
    }

    auto replyArgs = replyMessage.arguments();
    auto cookie = replyArgs.takeFirst().toUInt();
    KLOG_DEBUG() << "Receive cookie: " << cookie;
    return cookie;
}

uint32_t ScreenSaver::lockAndThrottle(const QString& reason)
{
    KLOG_DEBUG() << "Lock and throttle, reason: " << reason;

    RETURN_VAL_IF_FALSE(this->lock(), 0);
    return this->addThrottle(reason);
}

bool ScreenSaver::removeThrottle(uint32_t cookie)
{
    KLOG_DEBUG() << "Remove cookie: " << cookie;

    auto sendMessage = QDBusMessage::createMethodCall(SCREENSAVER_DBUS_NAME,
                                                      SCREENSAVER_DBUS_OBJECT_PATH,
                                                      SCREENSAVER_DBUS_INTERFACE,
                                                      "UnThrottle");
    sendMessage << cookie;

    auto replyMessage = QDBusConnection::sessionBus().call(sendMessage, QDBus::Block, DBUS_TIMEOUT_MS);

    if (replyMessage.type() == QDBusMessage::ErrorMessage)
    {
        KLOG_WARNING() << "Call UnThrottle failed: " << replyMessage.errorMessage();
        return false;
    }
    return true;
}

bool ScreenSaver::poke()
{
    KLOG_DEBUG() << "Call poke to screensaver";

    auto sendMessage = QDBusMessage::createMethodCall(SCREENSAVER_DBUS_NAME,
                                                      SCREENSAVER_DBUS_OBJECT_PATH,
                                                      SCREENSAVER_DBUS_INTERFACE,
                                                      "SimulateUserActivity");

    auto replyMessage = QDBusConnection::sessionBus().call(sendMessage, QDBus::NoBlock);

    if (replyMessage.type() == QDBusMessage::ErrorMessage)
    {
        KLOG_WARNING() << "Call SimulateUserActivity failed: " << replyMessage.errorMessage();
        return false;
    }
    return true;
}

void ScreenSaver::init()
{
}
}  // namespace Kiran
