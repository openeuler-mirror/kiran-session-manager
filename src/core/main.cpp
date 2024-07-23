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

#include <unistd.h>
#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTranslator>
#include <QtGlobal>
#include <iostream>
#include "app/app-manager.h"
#include "client/client-manager.h"
#include "config.h"
#include "inhibitor-manager.h"
#include "lib/base/base.h"
#include "session-manager.h"
#include "utils.h"
#include "xsmp-server.h"

using namespace Kiran;

#define DBUS_LAUNCH_COMMAND "dbus-launch"

void startDBusSession(int argc, char **argv)
{
    int i = 0;

    RETURN_IF_TRUE(!qgetenv("DBUS_SESSION_BUS_ADDRESS").isEmpty());
    RETURN_IF_TRUE(QString(argv[0]).startsWith(DBUS_LAUNCH_COMMAND));

    char **new_argv = static_cast<char **>(g_malloc(argc + 3 * sizeof(*argv)));

    new_argv[0] = const_cast<char *>(DBUS_LAUNCH_COMMAND);
    new_argv[1] = const_cast<char *>("--exit-with-session");

    for (i = 0; i < argc; i++)
    {
        new_argv[i + 2] = argv[i];
    }

    new_argv[i + 2] = NULL;

    KLOG_DEBUG("Start session manager by dbus-launch.");

    if (!execvp(DBUS_LAUNCH_COMMAND, new_argv))
    {
        KLOG_WARNING("No session bus and could not exec dbus-launch: %s", g_strerror(errno));
    }
}

void initEnv()
{
    QMap<QString, QString> envs;

    auto key_regex = QRegularExpression("^[A-Za-z_][A-Za-z0-9_]*$", QRegularExpression::OptimizeOnFirstUsageOption);
    auto value_regex = QRegularExpression("^([[:blank:]]|[^[:cntrl:]])*$", QRegularExpression::OptimizeOnFirstUsageOption);

    auto system_environment = QProcessEnvironment::systemEnvironment();

    for (const auto &key : system_environment.keys())
    {
        auto value = system_environment.value(key);
        if (key_regex.match(key).hasMatch() && value_regex.match(value).hasMatch())
        {
            envs.insert(key, value);
            KLOG_DEBUG() << "Add environment: " << key << "=" << value;
        }
        else
        {
            KLOG_WARNING() << "Filter environment: " << key << "=" << value;
        }
    }
    Utils::getDefault()->setEnvs(envs);

    if (qgetenv("XDG_CURRENT_DESKTOP").isEmpty())
    {
        Utils::getDefault()->setEnv("XDG_CURRENT_DESKTOP", "KIRAN");
    }
}

int main(int argc, char *argv[])
{
    auto argv0 = QFileInfo(argv[0]);
    auto programName = argv0.baseName();

    if (klog_qt5_init(QString(), "kylinsec-session", PROJECT_NAME, programName) < 0)
    {
        fprintf(stderr, "Failed to init kiran-log.");
    }

    // wayland方式下可能需要自己启动dbus-daemon
    startDBusSession(argc, argv);

    QApplication app(argc, argv);
    QApplication::setApplicationName(programName);
    QApplication::setApplicationVersion(PROJECT_VERSION);

    QTranslator translator;

    if (!translator.load(QLocale(), qAppName(), ".", KSM_INSTALL_TRANSLATIONDIR, ".qm"))
    {
        KLOG_WARNING() << "Load translator failed!";
    }
    else
    {
        app.installTranslator(&translator);
    }

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption(QStringList({"s", "session-type"}),
                                        app.translate("main", "Specify a session type that contains required components."),
                                        app.translate("main", "SESSION_NAME"),
                                        QStringLiteral("kiran")));

    parser.addOption(QCommandLineOption(QStringList({"a", "autostart"}),
                                        app.translate("main", "Override standard autostart directories."),
                                        app.translate("main", "AUTOSTART_DIRS"),
                                        QStringLiteral("")));
    parser.process(app);

    initEnv();

    auto sessionType = parser.value(QStringLiteral("session-type"));
    KLOG_DEBUG() << "sessionType: " << sessionType;

    if (parser.isSet("autostart"))
    {
        auto autostartDirs = parser.value(QStringLiteral("autostart"));
        Utils::getDefault()->setAutostartDirs(autostartDirs);
    }

    AppManager::globalInit(sessionType);
    XsmpServer::globalInit();
    InhibitorManager::globalInit();
    ClientManager::globalInit(XsmpServer::getInstance());
    SessionManager::globalInit(AppManager::getInstance(),
                               ClientManager::getInstance(),
                               InhibitorManager::getInstance());

    SessionManager::getInstance()->start();

    auto retval = app.exec();

    SessionManager::globalDeinit();
    ClientManager::globalDeinit();
    InhibitorManager::globalDeinit();
    XsmpServer::globalDeinit();
    AppManager::globalDeinit();

    return retval;
}
