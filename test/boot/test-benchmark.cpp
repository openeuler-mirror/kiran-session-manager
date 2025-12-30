/**
 * Copyright (c) 2025 ~ 2026 KylinSec Co., Ltd.
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

#include "test-benchmark.h"
#include <QElapsedTimer>
#include <QGSettings>
#include <QProcess>
#include <QTimer>
#include "config.h"
#include "lib/base/base.h"
#include "src/core/app/app-manager.h"
#include "src/core/client/client-manager.h"
#include "src/core/display-server-monitor.h"
#include "src/core/inhibitor-manager.h"
#include "src/core/session-manager.h"
#include "src/core/utils.h"
#include "src/core/xsmp-server.h"

namespace Kiran
{

#define DEFAULT_XORG_DISPLAY ":6"
// 测试会话启动和登出次数
#define MAX_BOOT_COUNT 50

class XorgProcess : public QProcess
{
    Q_OBJECT

public:
    XorgProcess(QObject *parent) : QProcess(parent) {};

private:
    virtual void setupChildProcess()
    {
        // 将SIGUSR1信号设置为SIG_IGN，确保Xorg会执行NotifyParentProcess函数通知父进程初始化完毕
        signal(SIGUSR1, SIG_IGN);
    }
};

bool TestBenchmark::m_xorgProcessStarted = false;

void TestBenchmark::initTestCase()
{
    m_settings = new QGSettings(KSM_SCHEMA_ID, "", this);

    struct sigaction action;
    action.sa_sigaction = &TestBenchmark::startXorgProcessFinished;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGUSR1, &action, NULL);

    // 启动Xorg
    m_xorgProcess = new XorgProcess(this);
    m_xorgProcess->start("/usr/bin/Xephyr",
                         QStringList() << "-screen" << "1280x960" << DEFAULT_XORG_DISPLAY);
    qputenv("DISPLAY", DEFAULT_XORG_DISPLAY);

    // 启动dbus会话后端
    m_dbusSessionProcess = new QProcess(this);
    m_dbusSessionProcess->start("/usr/bin/dbus-daemon", QStringList() << "--print-address" << "--fork" << "--session");
    m_dbusSessionProcess->waitForFinished();
    auto dbusAddress = m_dbusSessionProcess->readAllStandardOutput().trimmed();
    qputenv("DBUS_SESSION_BUS_ADDRESS", dbusAddress);
    qputenv("XDG_CURRENT_DESKTOP", "KIRAN");

    if (klog_qt5_init(QString(), "kylinsec-session", PROJECT_NAME, QCoreApplication::applicationName()))
    {
        fprintf(stderr, "Failed to init kiran-log.");
    }
}

void TestBenchmark::cleanupTestCase()
{
    // 杀死Xorg进程
    m_xorgProcess->kill();
    m_dbusSessionProcess->kill();
}

void TestBenchmark::sessionBootTime()
{
    // Utils::getDefault()->initEnv();

    // 等待Xorg进程启动完毕
    while (!m_xorgProcessStarted)
    {
        usleep(100);
    }
    // 禁止退出窗口显示，避免人工干预
    auto showExitWindow = m_settings->get(KSM_SCHEMA_KEY_ALWAYS_SHOW_EXIT_WINDOW).toBool();
    m_settings->set(KSM_SCHEMA_KEY_ALWAYS_SHOW_EXIT_WINDOW, false);

    AppManager::globalInit("kiran");
    XsmpServer::globalInit();
    InhibitorManager::globalInit();
    ClientManager::globalInit(XsmpServer::getInstance());
    SessionManager::globalInit(AppManager::getInstance(),
                               ClientManager::getInstance(),
                               InhibitorManager::getInstance());

    for (int i = 0; i < MAX_BOOT_COUNT; i++)
    {
        m_sessionElapsedTimer.restart();
        SessionManager::getInstance()->start();
        auto startFinishedConnect = connect(SessionManager::getInstance(), &SessionManager::PhaseChanged,
                                            [this](int phase)
                                            {
                                                if (phase == KSM_PHASE_RUNNING)
                                                {
                                                    QTimer::singleShot(100,
                                                                       [this]()
                                                                       {
                                                                           SessionManager::getInstance()->Logout(0);
                                                                       });
                                                };
                                            });

        QCoreApplication::exec();
        QCOMPARE(m_sessionElapsedTimer.elapsed() < 15000, true);
        QObject::disconnect(startFinishedConnect);
    }

    // 还原设置
    m_settings->set(KSM_SCHEMA_KEY_ALWAYS_SHOW_EXIT_WINDOW, showExitWindow);

    SessionManager::globalDeinit();
    ClientManager::globalDeinit();
    InhibitorManager::globalDeinit();
    XsmpServer::globalDeinit();
    AppManager::globalDeinit();
}

void TestBenchmark::startXorgProcessFinished(int signum, siginfo_t *info, void *data)
{
    if (info->si_signo == SIGUSR1)
    {
        m_xorgProcessStarted = true;
    }
}

}  // namespace Kiran

QTEST_MAIN(Kiran::TestBenchmark)

#include "test-benchmark.moc"