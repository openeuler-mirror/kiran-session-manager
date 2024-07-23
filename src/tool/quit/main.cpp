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
 * Author:     yangfeng <yangfeng@kylinsec.com.cn>
 */

#include <qt5-log-i.h>
#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QTranslator>
#include "config.h"
#include "ksm-i.h"
#include "session_manager_proxy.h"

int main(int argc, char *argv[])
{
    auto argv0 = QFileInfo(argv[0]);
    auto programName = argv0.baseName();

    if (klog_qt5_init(QString(), "kylinsec-session", PROJECT_NAME, programName) < 0)
    {
        fprintf(stderr, "Failed to init kiran-log.");
    }

    QApplication app(argc, argv);

    QTranslator translator;
    if (!translator.load(QLocale(), qAppName(), ".", KSM_INSTALL_TRANSLATIONDIR, ".qm"))
    {
        KLOG_WARNING() << "Load translator failed!";
    }
    else
    {
        app.installTranslator(&translator);
    }

    QTextStream cerr(stderr);

    QCommandLineParser parser;

    QCommandLineOption logoutOption("logout", QObject::tr("logout"));
    QCommandLineOption powerOffOption("power-off", QObject::tr("power-off"));
    QCommandLineOption rebootOption("reboot", QObject::tr("reboot"));

    parser.addOption(logoutOption);
    parser.addOption(powerOffOption);
    parser.addOption(rebootOption);
    parser.parse(app.arguments());

    SessionManagerProxy *sessionManagerProxy;
    sessionManagerProxy = new SessionManagerProxy(KSM_DBUS_NAME,
                                                  KSM_DBUS_OBJECT_PATH,
                                                  QDBusConnection::sessionBus());
    if (parser.isSet(logoutOption))
    {
        auto reply = sessionManagerProxy->Logout(0);
        reply.waitForFinished();
        if (reply.isError())
        {
            KLOG_WARNING() << "logout failed through dbus:" << reply.error().message();
            return 1;
        }
    }
    else if (parser.isSet(powerOffOption))
    {
        auto reply = sessionManagerProxy->Shutdown();
        reply.waitForFinished();
        if (reply.isError())
        {
            KLOG_WARNING() << "shutdown failed through dbus:" << reply.error().message();
            return 1;
        }
    }
    else if (parser.isSet(rebootOption))
    {
        auto reply = sessionManagerProxy->Reboot();
        reply.waitForFinished();
        if (reply.isError())
        {
            KLOG_WARNING() << "reboot failed through dbus:" << reply.error().message();
            return 1;
        }
    }
    else
    {
        cerr << parser.helpText() << Qt::endl;
        delete sessionManagerProxy;
        return 1;
    }

    delete sessionManagerProxy;

    return 0;
}