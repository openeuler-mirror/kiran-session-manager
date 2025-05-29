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

#include <KConfigGroup>
#include <KDesktopFile>
#include <KIO/DesktopExecParser>
#include <KService>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>
#include "lib/base/base.h"

#include "src/core/app/app-manager.h"
#include "src/core/app/app.h"
#include "src/core/utils.h"
#include <QUrl>

namespace Kiran
{
#define KSM_AUTOSTART_APP_ENABLED_KEY "X-KIRAN-Autostart-enabled"
#define KSM_AUTOSTART_APP_PHASE_KEY "X-KIRAN-Autostart-Phase"
#define KSM_AUTOSTART_APP_AUTORESTART_KEY "X-KIRAN-AutoRestart"
#define KSM_AUTOSTART_APP_DELAY_KEY "X-KIRAN-Autostart-Delay"

// 暂时兼容MATE应用
#define GSM_AUTOSTART_APP_ENABLED_KEY "X-MATE-Autostart-enabled"
#define GSM_AUTOSTART_APP_AUTORESTART_KEY "X-MATE-AutoRestart"
#define GSM_AUTOSTART_APP_PHASE_KEY "X-MATE-Autostart-Phase"

#define DESKTOP_KEY_DBUS_ACTIVATABLE "DBusActivatable"
#define DESKTOP_KEY_STARTUP_NOTIFY "StartupNotify"

App::App(const QString &filePath, QObject *parent = nullptr) : QObject(parent),
                                                               m_phase(KSMPhase::KSM_PHASE_APPLICATION),
                                                               m_autoRestart(false),
                                                               m_delay(0),
                                                               m_process(new QProcess(this))
{
    this->m_appInfo = QSharedPointer<KDesktopFile>(new KDesktopFile(filePath));
    m_process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
    connect(this->m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFinished(int, QProcess::ExitStatus)));
    this->loadAppInfo();
}

App::~App()
{
    KLOG_DEBUG() << "App " << this->m_appID << " is destroyed.";
}

bool App::start()
{
    /* DesktopAppInfo的接口不太完善，如果通过DBUS激活应用则拿不到进程ID，需要自己去DBUS总线获取，但是
       DesktopAppInfo又未暴露应用的dbus name，所以暂时先不支持DBUS启动。*/

    KLOG_DEBUG() << "Start app " << this->m_appInfo->fileName();

    if (this->m_process->state() != QProcess::ProcessState::NotRunning)
    {
        KLOG_WARNING() << "The process already exists.";
        return false;
    }

    if (this->m_appInfo->desktopGroup().readEntry(DESKTOP_KEY_DBUS_ACTIVATABLE, false))
    {
        KLOG_WARNING() << "DBus startup mode is not supported at present.";
        return false;
    }

    KService service(this->m_appInfo->fileName());

    auto arguments = KIO::DesktopExecParser(service, QList<QUrl>()).resultingArguments();
    if (arguments.isEmpty())
    {
        KLOG_WARNING() << "Failed to parse arguments for " << this->m_appInfo->fileName();
        return false;
    }

    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("DESKTOP_AUTOSTART_ID", this->m_startupID);
    this->m_process->setProcessEnvironment(env);

    auto program = arguments.takeFirst();
    this->m_process->start(program, arguments, QProcess::NotOpen);

    return true;
}

bool App::restart()
{
    KLOG_DEBUG() << "Restart app " << this->m_appInfo->fileName();

    this->stop();
    return this->start();
}

bool App::stop()
{
    KLOG_DEBUG() << "Stop app " << this->m_appInfo->fileName();

    if (this->m_process->state() == QProcess::ProcessState::NotRunning)
    {
        KLOG_WARNING() << "The app " << this->m_appInfo->fileName() << " is not running.";
        return false;
    }

    this->m_process->terminate();

    return true;
}

bool App::canLaunched()
{
    if (!this->m_appInfo->desktopGroup().readEntry(KSM_AUTOSTART_APP_ENABLED_KEY, true))
    {
        KLOG_DEBUG() << "The app " << this->m_appInfo->fileName() << " is disabled.";
        return false;
    }

    if (!this->m_appInfo->desktopGroup().readEntry(GSM_AUTOSTART_APP_ENABLED_KEY, true))
    {
        KLOG_DEBUG() << "The app " << this->m_appInfo->fileName() << " is disabled.";
        return false;
    }

    if (this->m_appInfo->desktopGroup().readEntry("Hidden", false))
    {
        KLOG_DEBUG() << "The app " << this->m_appInfo->fileName() << " is hidden.";
        return false;
    }

    auto showList = this->m_appInfo->desktopGroup().readXdgListEntry("OnlyShowIn");

    if (showList.length() > 0 && !showList.contains(DESKTOP_ENVIRONMENT) && !showList.contains("MATE"))
    {
        KLOG_DEBUG() << "The app " << this->m_appInfo->fileName() << " doesn't display in the desktop environment.";
        return false;
    }

    return true;
}

int32_t App::phaseStr2enum(const QString &phase_str)
{
    switch (shash(phase_str.toStdString().c_str()))
    {
    case "DisplayServer"_hash:
        return KSMPhase::KSM_PHASE_DISPLAY_SERVER;
    case "PostDisplayServer"_hash:
        return KSMPhase::KSM_PHASE_POST_DISPLAY_SERVER;
    case "Initialization"_hash:
        return KSMPhase::KSM_PHASE_INITIALIZATION;
    case "WindowManager"_hash:
        return KSMPhase::KSM_PHASE_WINDOW_MANAGER;
    case "Panel"_hash:
        return KSMPhase::KSM_PHASE_PANEL;
    case "Desktop"_hash:
        return KSMPhase::KSM_PHASE_DESKTOP;
    default:
        return KSMPhase::KSM_PHASE_APPLICATION;
    }
    return KSMPhase::KSM_PHASE_APPLICATION;
}

QString App::phaseEnum2str(int32_t phase)
{
    switch (phase)
    {
    case KSMPhase::KSM_PHASE_IDLE:
        return "Idle";
    case KSMPhase::KSM_PHASE_DISPLAY_SERVER:
        return "DisplayServer";
    case KSMPhase::KSM_PHASE_POST_DISPLAY_SERVER:
        return "PostDisplayServer";
    case KSMPhase::KSM_PHASE_INITIALIZATION:
        return "Initialization";
    case KSMPhase::KSM_PHASE_WINDOW_MANAGER:
        return "WindowManager";
    case KSMPhase::KSM_PHASE_PANEL:
        return "Panel";
    case KSMPhase::KSM_PHASE_DESKTOP:
        return "Desktop";
    case KSMPhase::KSM_PHASE_APPLICATION:
        return "Application";
    case KSMPhase::KSM_PHASE_RUNNING:
        return "Running";
    case KSMPhase::KSM_PHASE_QUERY_END_SESSION:
        return "QueryEndSession";
    case KSMPhase::KSM_PHASE_END_SESSION_PHASE1:
        return "EndSessionPhase1";
    case KSMPhase::KSM_PHASE_END_SESSION_PHASE2:
        return "EndSessionPhase2";
    case KSMPhase::KSM_PHASE_EXIT:
        return "Exit";
    default:
        KLOG_WARNING("The phase %d is invalid.", phase);
        return QString();
    }
    return QString();
}

void App::loadAppInfo()
{
    RETURN_IF_FALSE(this->m_appInfo);

    this->m_startupID = Utils::getDefault()->generateStartupID();

    // Desktop ID
    this->m_appID = QFileInfo(this->m_appInfo->fileName()).fileName();

    // Phase
    auto phase = this->m_appInfo->desktopGroup().readEntry(KSM_AUTOSTART_APP_PHASE_KEY);
    if (phase.isEmpty())
    {
        phase = this->m_appInfo->desktopGroup().readEntry(GSM_AUTOSTART_APP_PHASE_KEY);
    }
    this->m_phase = App::phaseStr2enum(phase);

    // Auto restart
    if (this->m_appInfo->desktopGroup().hasKey(KSM_AUTOSTART_APP_AUTORESTART_KEY))
    {
        this->m_autoRestart = this->m_appInfo->desktopGroup().readEntry(KSM_AUTOSTART_APP_AUTORESTART_KEY, false);
    }
    else
    {
        this->m_autoRestart = this->m_appInfo->desktopGroup().readEntry(GSM_AUTOSTART_APP_AUTORESTART_KEY, false);
    }

    // Delay
    auto delay = this->m_appInfo->desktopGroup().readEntry(KSM_AUTOSTART_APP_DELAY_KEY);
    if (!delay.isEmpty())
    {
        /* 由于在APPLICATION阶段之前会话管理会设置一个超时定时器在应用程序启动时等待一段时间，
        如果此时设置的延时启动时间大于这个定时器等待的时间，可能会发生一些不可预知的后果，
        因此在APPLICATION阶段之前启动的应用程序暂时不运行设置延时启动。如果一定要设置，那
        这个时间应该要小于会话管理设置的超时定时器的时间。*/
        if (this->m_phase == KSMPhase::KSM_PHASE_APPLICATION)
        {
            this->m_delay = delay.toInt();
        }
        else
        {
            KLOG_WARNING() << "Only accept an delay in KSM_PHASE_APPLICATION phase.";
        }
    }

    KLOG_DEBUG() << "The info of app " << this->m_appID << "is loaded. StartupID: "
                 << this->m_startupID << ", Phase: " << App::phaseEnum2str(this->m_phase)
                 << ", AutoRestart: " << this->m_autoRestart << ", Delay: " << this->m_delay;
}

void App::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    switch (exitStatus)
    {
    case QProcess::ExitStatus::NormalExit:
        KLOG_DEBUG() << "The app " << this->m_appInfo->fileName() << " normal exits.";
        break;
    case QProcess::ExitStatus::CrashExit:
        KLOG_DEBUG() << "The app " << this->m_appInfo->fileName() << " abnormal normal exits, exit code " << exitCode;
        break;
    default:
        break;
    }

    Q_EMIT this->AppExited();
}
}  // namespace Kiran
