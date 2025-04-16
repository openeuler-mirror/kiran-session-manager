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

#include <KDesktopFile>
#include <QEnableSharedFromThis>
#include <QProcess>

namespace Kiran
{
enum KSMPhase
{
    // 未开始
    KSM_PHASE_IDLE = 0,
    // 启动显示服务器(xserver/wayland-server)
    KSM_PHASE_DISPLAY_SERVER,
    // (IdleMonitor)
    KSM_PHASE_POST_DISPLAY_SERVER,
    // 启动控制中心会话后端
    KSM_PHASE_INITIALIZATION,
    // 启动窗口管理器
    KSM_PHASE_WINDOW_MANAGER,
    // 启动底部面板
    KSM_PHASE_PANEL,
    // 启动文件管理器
    KSM_PHASE_DESKTOP,
    // 启动其他应用程序
    KSM_PHASE_APPLICATION,
    // 会话运行阶段
    KSM_PHASE_RUNNING,
    // 会话结束阶段
    KSM_PHASE_QUERY_END_SESSION,
    // 会话结束第一阶段
    KSM_PHASE_END_SESSION_PHASE1,
    // 会话结束第二阶段
    KSM_PHASE_END_SESSION_PHASE2,
    // 会话结束第二阶段
    KSM_PHASE_EXIT
};

class App : public QObject
{
    Q_OBJECT

public:
    App(const QString &filePath, QObject *parent);
    virtual ~App();

    bool start();
    bool restart();
    bool stop();
    bool isRunning();
    bool canLaunched();

    //
    static int32_t phaseStr2enum(const QString &phase_str);
    static QString phaseEnum2str(int32_t phase);

    QString getAppID() { return this->m_appID; };
    int32_t getPhase() { return this->m_phase; };
    QString getStartupID() { return this->m_startupID; };
    bool getAutoRestart() { return this->m_autoRestart; };
    int32_t getDelay() { return this->m_delay; };

Q_SIGNALS:
    void AppExited();

private:
    void loadAppInfo();

    // void startup_notification();

private Q_SLOTS:
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);

    // static void on_launch_cb(GDesktopAppInfo *appinfo, GPid pid, gpointer user_data);
    // static void on_app_exited_cb(GPid pid, gint status, gpointer user_data);

private:
    QString m_appID;
    int32_t m_phase;
    QString m_startupID;
    // 正常退出后是否自动重启
    bool m_autoRestart;
    // 延时运行时间
    int32_t m_delay;

    QSharedPointer<KDesktopFile> m_appInfo;
    QProcess *m_process;

    // sigc::signal<void> m_appExited;
};

}  // namespace Kiran
