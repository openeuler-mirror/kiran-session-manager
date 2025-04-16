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

#include "src/core/utils.h"
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QStandardPaths>
#include "lib/base/base.h"

namespace Kiran
{
/* The XSMP spec defines the ID as:
 *
 * Version: "1"
 * Address type and address:
 *   "1" + an IPv4 address as 8 hex digits
 *   "2" + a DECNET address as 12 hex digits
 *   "6" + an IPv6 address as 32 hex digits
 * Time stamp: milliseconds since UNIX epoch as 13 decimal digits
 * Process-ID type and process-ID:
 *   "1" + POSIX PID as 10 decimal digits
 * Sequence number as 4 decimal digits
 *
 * XSMP client IDs are supposed to be globally unique: if
 * SmsGenerateClientID() is unable to determine a network
 * address for the machine, it gives up and returns %NULL.
 * MATE and KDE have traditionally used a fourth address
 * format in this case:
 *   "0" + 16 random hex digits
 *
 * We don't even bother trying SmsGenerateClientID(), since the
 * user's IP address is probably "192.168.1.*" anyway, so a random
 * number is actually more likely to be globally unique.
 */

Utils::Utils() : m_useCustomAutostartDirs(false)
{
    qDBusRegisterMetaType<QMap<QString, QString>>();
}

QSharedPointer<Utils> Utils::m_instance = nullptr;
QSharedPointer<Utils> Utils::getDefault()
{
    if (!m_instance)
    {
        m_instance = QSharedPointer<Utils>::create();
    }
    return m_instance;
}

QString Utils::generateStartupID()
{
    static int sequence = -1;
    static uint32_t rand1 = 0;
    static uint32_t rand2 = 0;
    static pid_t pid = 0;
    struct timeval tv;

    if (!rand1)
    {
        rand1 = rand();
        rand2 = rand();
        pid = getpid();
    }

    sequence = (sequence + 1) % 10000;
    gettimeofday(&tv, NULL);
    return QString::asprintf("10%.04x%.04x%.10lu%.3u%.10lu%.4d",
                             rand1,
                             rand2,
                             (unsigned long)tv.tv_sec,
                             (unsigned)tv.tv_usec,
                             (unsigned long)pid,
                             sequence);
}

void Utils::setAutostartDirs(const QString &autostartDirs)
{
    m_customAutostartDirs = autostartDirs.split(';');
    m_useCustomAutostartDirs = true;
}

QStringList Utils::getAutostartDirs()
{
    if (m_useCustomAutostartDirs)
    {
        return m_customAutostartDirs;
    }

    QStringList autostartDirs;
    auto configDirs = QStandardPaths::standardLocations(QStandardPaths::StandardLocation::ConfigLocation);

    for (auto &iter : configDirs)
    {
        autostartDirs.push_back(iter + "/autostart");
    }

    return autostartDirs;
}

void Utils::setEnv(const QString &name, const QString &value)
{
    KLOG_DEBUG() << "Name: " << name << ", value: " << value;

    qputenv(name.toStdString().c_str(), value.toUtf8());

    QMap<QString, QString> envs{{name, value}};
    setEnvs(envs);
}

void Utils::setEnvs(const QMap<QString, QString> &envs)
{
    QStringList listEnv;
    for (const auto &key : envs.keys())
    {
        auto env = QString("%1=%2").arg(key).arg(envs.value(key));
        listEnv.push_back(env);
    }

    KLOG_DEBUG() << "Set environments: " << listEnv.join(QStringLiteral(";"));

    {
        auto sendMessage = QDBusMessage::createMethodCall(DAEMON_DBUS_NAME,
                                                          DAEMON_DBUS_OBJECT_PATH,
                                                          DAEMON_DBUS_INTERFACE_NAME,
                                                          "UpdateActivationEnvironment");
        sendMessage << QVariant::fromValue(envs);
        auto replyMessage = QDBusConnection::sessionBus().call(sendMessage, QDBus::Block, DBUS_TIMEOUT_MS);

        if (replyMessage.type() == QDBusMessage::ErrorMessage)
        {
            KLOG_WARNING() << "Call UpdateActivationEnvironment failed: " << replyMessage.errorMessage();
        }
    }

    {
        auto sendMessage = QDBusMessage::createMethodCall(SYSTEMD_DBUS_NAME,
                                                          SYSTEMD_DBUS_OBJECT_PATH,
                                                          SYSTEMD_DBUS_INTERFACE_NAME,
                                                          "SetEnvironment");
        sendMessage << QVariant::fromValue(listEnv);
        auto replyMessage = QDBusConnection::sessionBus().call(sendMessage, QDBus::Block, DBUS_TIMEOUT_MS);

        if (replyMessage.type() == QDBusMessage::ErrorMessage)
        {
            KLOG_WARNING() << "Call SetEnvironment failed: " << replyMessage.errorMessage();
        }
    }
}

uint32_t Utils::generateCookie()
{
    return rand();
}

}  // namespace Kiran