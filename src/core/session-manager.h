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

#include <QDBusContext>
#include <QDBusObjectPath>
#include <QList>
#include <QObject>
#include "ksm-i.h"

class SessionManagerAdaptor;
class QGSettings;
class QTimer;
class QProcess;

namespace Kiran
{
class AppManager;
class ClientManager;
class Inhibitor;
class InhibitorManager;
class ExitQueryWindow;
class App;
class Client;
class Power;

class SessionManager : public QObject,
                       protected QDBusContext
{
    Q_OBJECT

    Q_PROPERTY(bool ScreenLockedWhenHibernate READ screenLockedWhenHibernate WRITE setScreenLockedWhenHibernate)
    Q_PROPERTY(bool ScreenLockedWhenSuspend READ screenLockedWhenSuspend WRITE setScreenLockedWhenSuspend)
public:
    SessionManager(AppManager *appManager,
                   ClientManager *clientManager,
                   InhibitorManager *inhibitorManager);
    virtual ~SessionManager();

    static SessionManager *getInstance() { return m_instance; };

    static void globalInit(AppManager *appManager,
                           ClientManager *clientManager,
                           InhibitorManager *inhibitorManager);

    static void globalDeinit() { delete m_instance; };

    // 会话开始
    void start();

    bool screenLockedWhenHibernate();
    void setScreenLockedWhenHibernate(bool screenLockedWhenHibernate);

    bool screenLockedWhenSuspend();
    void setScreenLockedWhenSuspend(bool screenLockedWhenSuspend);

public Q_SLOTS:  // METHODS
    bool CanHibernate();
    bool CanLogout();
    bool CanReboot();
    bool CanShutdown();
    bool CanSuspend();
    // 获取抑制器
    QString GetInhibitor(uint cookie);
    QString GetInhibitors();
    void Hibernate();
    // 添加抑制器
    uint Inhibit(const QString &appID, uint toplevelXID, const QString &reason, uint flags);
    // 判断指定flags的抑制器是否存在
    bool IsInhibited(uint flags);
    void Logout(uint mode);
    void Reboot();
    QDBusObjectPath RegisterClient(const QString &appID, const QString &clientStartupID);
    void RequestReboot();
    void RequestShutdown();
    // 添加会话程序的环境变量
    void Setenv(const QString &name, const QString &value);
    void Shutdown();
    void Suspend();
    // 删除抑制器
    void Uninhibit(uint inhibitCookie);
Q_SIGNALS:  // SIGNALS
    void InhibitorAdded(uint cookie);
    void InhibitorRemoved(uint cookie);
    void PhaseChanged(int phase);
    void ScreenLockedWhenHibernateChanged(bool screen_locked_when_hibernate);
    void ScreenLockedWhenSuspendChanged(bool screen_locked_when_suspend);

public Q_SLOTS:
    // 应用启动超时
    void onPhaseStartupTimeout();
    void onAppExited(App *app);
    void onClientAdded(Client *client);
    void onClientChanged(Client *client);
    void onClientDeleted(Client *client);
    void onInteractRequest(Client *client);
    void onInteractDone(Client *client);
    // xsmp客户端请求取消结束会话响应
    void onShutdownCanceled(Client *client);
    void onEndSessionPhase2Request(Client *client);
    void onEndSessionResponse(Client *client);

    void onInhibitorAdded(QSharedPointer<Inhibitor> inhibitor);
    void onInhibitorDeleted(QSharedPointer<Inhibitor> inhibitor);

    // 退出会话确认对话框响应
    void onExitWindowResponse();
    // 系统信号处理
    void onSystemSignal(int signo);

private:
    void init();

    void processPhase();
    // 开始阶段
    void processPhaseStartup();
    // 准备结束会话，此时会询问已注册的客户端会话是否可以结束，客户端收到应该进行响应
    void processPhaseQueryEndSession();
    // 询问客户端会话是否结束处理完毕
    void queryEndSessionComplete();
    // 取消结束会话
    void cancelEndSession();
    // 会话结束第一阶段
    void processPhaseEndSessionPhase1();
    // 会话结束第二阶段
    void processPhaseEndSessionPhase2();
    // 退出阶段
    void processPhaseExit();

    // 如果是当前账户的最后一个会话，需要停止会话中的dbus程序
    void maybeRestartUserBus();
    // 开始下一个阶段
    void startNextPhase();
    // 处理application阶段结束
    void processPhaseApplicationEnd();

    // 所有跟客户端的交互都已经处理完毕，开始真正的退出会话
    void quitSession();
    // 触发登出动作
    void doPowerAction(PowerAction power_action);

    // KSMLogoutAction logout_type2action(KSMPowerAction logout_type);

    /* ----------------- 以下为回调处理函数 ------------------- */
    // 应用已启动完成回调
    void onAppStartupFinished(App *app);
    void onWaitingSessionTimeout(std::function<void(void)> phase_complete_callback);

private:
    static SessionManager *m_instance;

    SessionManagerAdaptor *m_dbusAdaptor;

    AppManager *m_appManager;
    ClientManager *m_clientManager;
    InhibitorManager *m_inhibitorManager;

    QGSettings *m_settings;

    // 会话运行的阶段
    int32_t m_currentPhase;

    // 等待启动完成的应用
    QList<App *> m_waitingApps;
    QTimer *m_waitingAppsTimeoutID;

    // 等待响应的客户端列表
    QList<Client *> m_waitingClients;
    QTimer *m_waitingClientsTimeoutID;
    // 请求进入第二阶段的客户端列表
    QList<Client *> phase2_request_clients_;

    Power *m_power;
    // 抑制对话框
    // std::shared_ptr<ExitQueryWindow> exit_query_window_;
    PowerAction m_powerAction;

    QProcess *m_process;
};
}  // namespace Kiran