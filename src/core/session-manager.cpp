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

#include "src/core/session-manager.h"
#include <signal.h>
#include <QGSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QProcess>
#include <QTimer>
#include "lib/base/base.h"
#include "lib/dbus/systemd-login1.h"
#include "src/core/app/app-manager.h"
#include "src/core/client/client-manager.h"
#include "src/core/inhibitor-manager.h"
#include "src/core/power.h"
#include "src/core/presence.h"
#include "src/core/session_manager_adaptor.h"
#include "src/core/signal-handler.h"
#include "src/core/utils.h"

namespace Kiran
{
// 在一个阶段等待所有应用的最长时间(秒)
#define KSM_PHASE_STARTUP_TIMEOUT 15

SessionManager::SessionManager(AppManager *app_manager,
                               ClientManager *client_manager,
                               InhibitorManager *inhibitor_manager) : QObject(nullptr),
                                                                      m_appManager(app_manager),
                                                                      m_clientManager(client_manager),
                                                                      m_inhibitorManager(inhibitor_manager),
                                                                      m_currentPhase(KSMPhase::KSM_PHASE_IDLE),
                                                                      m_powerAction(PowerAction::POWER_ACTION_NONE)
{
    this->m_dbusAdaptor = new SessionManagerAdaptor(this);
    this->m_settings = new QGSettings(KSM_SCHEMA_ID, "", this);
    this->m_power = new Power(this);
    this->m_process = new QProcess(this);

    this->m_waitingAppsTimeoutID = new QTimer(this);
    this->m_waitingClientsTimeoutID = new QTimer(this);
}

SessionManager::~SessionManager()
{
}

SessionManager *SessionManager::m_instance = nullptr;
void SessionManager::globalInit(AppManager *appManager,
                                ClientManager *clientManager,
                                InhibitorManager *inhibitorManager)
{
    m_instance = new SessionManager(appManager, clientManager, inhibitorManager);
    m_instance->init();
}

void SessionManager::start()
{
    this->m_currentPhase = KSMPhase::KSM_PHASE_DISPLAY_SERVER;
    this->processPhase();
}

bool SessionManager::CanLogout()
{
    return true;
}

bool SessionManager::CanReboot()
{
    return this->m_power->canPowerAction(PowerAction::POWER_ACTION_REBOOT);
}

bool SessionManager::CanShutdown()
{
    return this->m_power->canPowerAction(PowerAction::POWER_ACTION_SHUTDOWN);
}

QString SessionManager::GetInhibitor(uint cookie)
{
    QJsonDocument jsonDoc;
    QJsonObject jsonObj;

    auto inhibitor = InhibitorManager::getInstance()->getInhibitor(cookie);

    if (inhibitor)
    {
        jsonObj = QJsonObject{
            {KSM_INHIBITOR_JK_COOKIE, int(inhibitor->cookie)},
            {KSM_INHIBITOR_JK_APP_ID, inhibitor->appID},
            {KSM_INHIBITOR_JK_TOPLEVEL_XID, int(inhibitor->toplevelXID)},
            {KSM_INHIBITOR_JK_REASON, inhibitor->reason},
            {KSM_INHIBITOR_JK_FLAGS, int(inhibitor->flags)},
        };
    }

    jsonDoc.setObject(jsonObj);
    return QString(jsonDoc.toJson());
}

QString SessionManager::GetInhibitors()
{
    QJsonDocument jsonDoc;
    QJsonArray jsonArr;

    int32_t i = 0;
    for (auto inhibitor : this->m_inhibitorManager->getInhibitors())
    {
        QJsonObject jsonObj{
            {KSM_INHIBITOR_JK_COOKIE, int(inhibitor->cookie)},
            {KSM_INHIBITOR_JK_APP_ID, inhibitor->appID},
            {KSM_INHIBITOR_JK_TOPLEVEL_XID, int(inhibitor->toplevelXID)},
            {KSM_INHIBITOR_JK_REASON, inhibitor->reason},
            {KSM_INHIBITOR_JK_FLAGS, int(inhibitor->flags)},
        };

        jsonArr.push_back(jsonObj);
    }

    jsonDoc.setArray(jsonArr);
    return QString(jsonDoc.toJson());
}

uint SessionManager::SessionManager::Inhibit(const QString &appID,
                                             uint toplevelXID,
                                             const QString &reason,
                                             uint flags)
{
    KLOG_DEBUG() << "App id: " << appID << ", toplevel xid: " << toplevelXID << ", reason: " << reason << ", flags: " << flags;

    auto inhibitor = this->m_inhibitorManager->addInhibitor(appID, toplevelXID, reason, flags);

    if (!inhibitor)
    {
        DBUS_ERROR_REPLY(QDBusError::Failed, KSMErrorCode::ERROR_MANAGER_GEN_UNIQUE_COOKIE_FAILED);
        return 0;
    }

    return inhibitor->cookie;
}

bool SessionManager::IsInhibited(uint flags)
{
    return InhibitorManager::getInstance()->hasInhibitor(flags);
}

void SessionManager::Logout(uint mode)
{
    if (this->m_currentPhase > KSMPhase::KSM_PHASE_RUNNING)
    {
        DBUS_ERROR_REPLY_AND_RET(QDBusError::InvalidArgs, KSMErrorCode::ERROR_MANAGER_PHASE_INVALID);
    }

    if (!this->m_power->canPowerAction(PowerAction::POWER_ACTION_LOGOUT))
    {
        DBUS_ERROR_REPLY_AND_RET(QDBusError::AccessDenied, KSMErrorCode::ERROR_MANAGER_POWER_ACTION_UNSUPPORTED);
    }

    this->m_powerAction = PowerAction::POWER_ACTION_LOGOUT;
    this->startNextPhase();
}

void SessionManager::Reboot()
{
    if (this->m_currentPhase > KSMPhase::KSM_PHASE_RUNNING)
    {
        DBUS_ERROR_REPLY_AND_RET(QDBusError::InvalidArgs, KSMErrorCode::ERROR_MANAGER_PHASE_INVALID);
    }

    if (!this->m_power->canPowerAction(PowerAction::POWER_ACTION_REBOOT))
    {
        DBUS_ERROR_REPLY_AND_RET(QDBusError::AccessDenied, KSMErrorCode::ERROR_MANAGER_POWER_ACTION_UNSUPPORTED);
    }

    this->m_powerAction = PowerAction::POWER_ACTION_REBOOT;
    this->startNextPhase();
}

QDBusObjectPath SessionManager::RegisterClient(const QString &appID, const QString &clientStartupID)
{
    KLOG_DEBUG() << "App id: " << appID << " , startup id:  " << clientStartupID;

    if (this->m_currentPhase > KSMPhase::KSM_PHASE_RUNNING)
    {
        DBUS_ERROR_REPLY(QDBusError::InvalidArgs, KSMErrorCode::ERROR_MANAGER_PHASE_CANNOT_REGISTER);
        return QDBusObjectPath();
    }

    auto client = this->m_clientManager->addClientDBus(clientStartupID, this->message().service(), appID);

    if (!client)
    {
        DBUS_ERROR_REPLY(QDBusError::Failed, KSMErrorCode::ERROR_MANAGER_CLIENT_ALREADY_REGISTERED, clientStartupID.toUtf8().data());
        return QDBusObjectPath();
    }

    return client->getObjectPath();
}

void SessionManager::RequestReboot()
{
    this->Reboot();
}

void SessionManager::RequestShutdown()
{
    this->Shutdown();
}

void SessionManager::Setenv(const QString &name, const QString &value)
{
    KLOG_DEBUG() << "Set environment variable. name: " << name << ", value: " << value;

    Utils::getDefault()->setEnv(name, value);
}

void SessionManager::Shutdown()
{
    if (this->m_currentPhase > KSMPhase::KSM_PHASE_RUNNING)
    {
        DBUS_ERROR_REPLY_AND_RET(QDBusError::InvalidArgs, KSMErrorCode::ERROR_MANAGER_PHASE_INVALID);
    }

    if (!this->m_power->canPowerAction(PowerAction::POWER_ACTION_SHUTDOWN))
    {
        DBUS_ERROR_REPLY_AND_RET(QDBusError::AccessDenied, KSMErrorCode::ERROR_MANAGER_POWER_ACTION_UNSUPPORTED);
    }

    this->m_powerAction = PowerAction::POWER_ACTION_SHUTDOWN;
    this->startNextPhase();
}

void SessionManager::Uninhibit(uint inhibitCookie)
{
    auto inhibitor = InhibitorManager::getInstance()->getInhibitor(inhibitCookie);
    if (!inhibitor)
    {
        DBUS_ERROR_REPLY_AND_RET(QDBusError::Failed, KSMErrorCode::ERROR_MANAGER_INHIBITOR_NOTFOUND);
    }
    InhibitorManager::getInstance()->deleteInhibitor(inhibitCookie);
}

void SessionManager::onPhaseStartupTimeout()
{
    switch (this->m_currentPhase)
    {
    case KSMPhase::KSM_PHASE_IDLE:
    case KSMPhase::KSM_PHASE_DISPLAY_SERVER:
    case KSMPhase::KSM_PHASE_POST_DISPLAY_SERVER:
    case KSMPhase::KSM_PHASE_INITIALIZATION:
    case KSMPhase::KSM_PHASE_WINDOW_MANAGER:
    case KSMPhase::KSM_PHASE_PANEL:
    case KSMPhase::KSM_PHASE_DESKTOP:
    case KSMPhase::KSM_PHASE_APPLICATION:
    {
        for (auto app : this->m_waitingApps)
        {
            KLOG_WARNING() << "Wait app " << app->getAppID() << " startup timeout.";
        }
        break;
    }
    default:
        break;
    }

    this->m_waitingAppsTimeoutID->stop();
    this->startNextPhase();
}

void SessionManager::onWaitingSessionTimeout(std::function<void(void)> phase_complete_callback)
{
    KLOG_WARNING() << "Wait session timeout.";

    for (auto client : this->m_waitingClients)
    {
        KLOG_WARNING() << "The client " << client->getID() << " doesn't response message. phase: " << App::phaseEnum2str(this->m_currentPhase);

        // 将交互超时的客户端加入抑制器
        this->m_inhibitorManager->addInhibitor(client->getAppID(),
                                               0,
                                               tr("This program is not responding"),
                                               KSMInhibitorFlag::KSM_INHIBITOR_FLAG_QUIT,
                                               client->getID());
    }

    phase_complete_callback();
    this->m_waitingClientsTimeoutID->disconnect();
    this->m_waitingClientsTimeoutID->stop();
}

void SessionManager::onAppExited(App *app)
{
    this->onAppStartupFinished(app);
}

void SessionManager::onClientAdded(Client *client)
{
    RETURN_IF_FALSE(client);

    KLOG_DEBUG() << "Client " << client->getID() << " added.";

    auto app = this->m_appManager->getAppByStartupID(client->getID());
    if (!app)
    {
        KLOG_DEBUG() << "Not found app for the client " << client->getID();
        return;
    }
    KLOG_DEBUG() << "The new client " << client->getID() << " match the app " << app->getAppID();

    // 此时说明kiran-session-daemon已经关闭掉与mate-settings-daemon冲突的插件，这时可以运行mate-settings-daemon
    auto iter = std::find_if(this->m_waitingApps.begin(), this->m_waitingApps.end(), [](App *app)
                             { return app->getAppID() == "mate-settings-daemon.desktop"; });
    if (app->getAppID() == "kiran-session-daemon.desktop" &&
        iter != this->m_waitingApps.end())
    {
        KLOG_DEBUG() << "Start to boot " << (*iter)->getAppID();
        (*iter)->start();
    }

    this->onAppStartupFinished(app);
}

void SessionManager::onClientChanged(Client *client)
{
    RETURN_IF_FALSE(client);

    // 通过xsmp协议添加客户端时，还不能获取到客户端的属性信息，所以需要在属性变化时再次进行判断
    KLOG_DEBUG() << "Client " << client->getID() << " changed.";

    auto appID = client->getAppID();
    if (!appID.isEmpty())
    {
        auto app = this->m_appManager->getApp(appID);
        if (!app)
        {
            KLOG_DEBUG() << "Not found app by " << appID;
            return;
        }
        KLOG_DEBUG() << "The client " << client->getID() << " match the app " << app->getAppID();
        this->onAppStartupFinished(app);
    }
}

void SessionManager::onClientDeleted(Client *client)
{
    RETURN_IF_FALSE(client);
    KLOG_DEBUG() << "Client " << client->getID() << " deleted, AppID: " << client->getAppID();
    // 客户端断开连接或者异常退出后无法再响应会话管理的请求，因此这里主动调用一次客户端响应回调函数来处理该客户端
    this->onEndSessionResponse(client);

    if (this->m_currentPhase == KSMPhase::KSM_PHASE_RUNNING)
    {
        auto app = this->m_appManager->getAppByStartupID(client->getID());

        /* QT应用程序不会使用会话管理通过DESKTOP_AUTOSTART_ID环境变量传递的值作为StartupID，在执行onRegisterClient回调时，
           QT传递的previousID为空，因此会话管理又重新生成了一个新的StartupID给QT程序，因此通过getAppByStartupID函数是无法找到
           对应App对象的，因此这里计划通过Xsmp协议中ProgramName属性来作为AppID，然后关联到App对象。*/
        if (!app)
        {
            app = this->m_appManager->getApp(client->getAppID());
        }

        if (!app)
        {
            KLOG_DEBUG() << "No corresponding app found for client " << client->getID();
        }

        /* 这里不调用restart而是start的原因是因为存在如下情况：
           当已经存在一个caja进程时，再次运行caja，caja会重新发送一个新的clientID（正常逻辑应该是发送第一次启动caja时的ClientID）
           而这个新发送的ClientID在稍后又被caja给关闭了，这样就会触发ClientDeleted信号，而且会触发两次，
           第一次是通过dbus协议发送(监听name lost信号)，第二次是通过x协议发送，在第二次时getAppID会查找到第一个clientID，
           这样就导致app被重启（而实际上caja并未退出），然后caja又会循环这样的操作，导致caja一直无法拉起 */
        if (app && app->getAutoRestart())
        {
            QTimer::singleShot(600, app, &App::start);
        }
    }
}

void SessionManager::onInteractRequest(Client *client)
{
    RETURN_IF_FALSE(this->m_currentPhase == KSMPhase::KSM_PHASE_QUERY_END_SESSION);

    // 需要跟用户进行交互，因此此处需要添加抑制器，不能马上进行退出操作

    this->m_inhibitorManager->addInhibitor(client->getAppID(),
                                           0,
                                           tr("This program is blocking exit"),
                                           KSMInhibitorFlag::KSM_INHIBITOR_FLAG_QUIT,
                                           client->getID());

    /* 当通过SmsSaveYourself向xsmp客户端发起QueryEndSession时，客户端可能要求与用户交互(InteractRequest)，也可能直接回复SaveYourselfDone，
       无论是那种情况，都表示客户端已经收到了会话管理发送的消息，因此应该将该客户端从等待队列中删除。这里为了方便直接调用on_end_session_response_cb
       函数进行处理了，如果后续需要针对这两个消息做不同的处理，此处的逻辑需要修改。*/
    this->onEndSessionResponse(client);
}

void SessionManager::onInteractDone(Client *client)
{
    this->m_inhibitorManager->deleteInhibitorByStartupID(client->getID());
}

void SessionManager::onShutdownCanceled(Client *client)
{
    KLOG_WARNING() << "Client: " << client->getID() << " want to cancels shutdown. ignore the client request.";
    /* 如果QT的窗口在closeEvent函数中调用event->ignore()来忽略窗口关闭事件，则桌面会话退出时会收到QT客户端发送的取消结束会话的事件，
       用于响应客户端取消结束会话的事件不能通知到用户，会导致开始菜单->注销按钮功能不能正常使用，影响用户体验，因此暂时禁止处理该请求。*/
    // this->cancelEndSession();
}

void SessionManager::onEndSessionPhase2Request(Client *client)
{
    KLOG_DEBUG() << "Receive client: " << client->getID() << " phase2 request.";

    // 需要第一阶段结束的所有客户端响应后才能进入第二阶段，因此这里先缓存第二阶段请求的客户端
    this->phase2_request_clients_.push_back(client);
}

void SessionManager::onEndSessionResponse(Client *client)
{
    RETURN_IF_FALSE(client);

    KLOG_DEBUG() << "Receive client " << client->getID() << " end session response. phase: " << App::phaseEnum2str(this->m_currentPhase);

    RETURN_IF_TRUE(this->m_currentPhase < KSMPhase::KSM_PHASE_QUERY_END_SESSION);

    auto iter = std::remove_if(this->m_waitingClients.begin(),
                               this->m_waitingClients.end(),
                               [client](Client *item)
                               {
                                   return client->getID() == item->getID();
                               });
    this->m_waitingClients.erase(iter, this->m_waitingClients.end());

    if (m_waitingClients.size() == 0)
    {
        switch (this->m_currentPhase)
        {
        case KSMPhase::KSM_PHASE_QUERY_END_SESSION:
            this->queryEndSessionComplete();
            break;
        case KSMPhase::KSM_PHASE_END_SESSION_PHASE1:
        case KSMPhase::KSM_PHASE_END_SESSION_PHASE2:
            this->startNextPhase();
            break;
        default:
            KLOG_WARNING() << "The phase is invalid. current phase: " << App::phaseEnum2str(this->m_currentPhase) << ", client id: " << client->getID();
            break;
        }
    }
}

void SessionManager::onInhibitorAdded(QSharedPointer<Inhibitor> inhibitor)
{
    Q_EMIT this->InhibitorAdded(inhibitor->cookie);
}

void SessionManager::onInhibitorDeleted(QSharedPointer<Inhibitor> inhibitor)
{
    Q_EMIT this->InhibitorRemoved(inhibitor->cookie);
}

void SessionManager::onExitWindowResponse()
{
    QJsonParseError jsonError;

    this->m_process->disconnect();

    auto standardOutput = this->m_process->readAllStandardOutput();

    KLOG_DEBUG() << "Standard output: " << standardOutput;

    auto jsonDoc = QJsonDocument::fromJson(standardOutput, &jsonError);

    if (jsonDoc.isNull())
    {
        KLOG_WARNING() << "Parser standard output failed: " << jsonError.errorString();
        this->cancelEndSession();
        return;
    }

    auto jsonRoot = jsonDoc.object();
    auto responseID = jsonRoot["response_id"].toString().toLower();

    switch (shash(responseID.toUtf8().data()))
    {
    case "ok"_hash:
        this->startNextPhase();
        break;
    default:
        this->cancelEndSession();
        break;
    }
}

void SessionManager::onSystemSignal(int signo)
{
    KLOG_DEBUG("Receive signal: %d.", signo);

    if (signo == SIGTERM)
    {
        this->quitSession();
    }
}

void SessionManager::init()
{
    this->m_power->init();

    this->m_waitingAppsTimeoutID->setInterval(KSM_PHASE_STARTUP_TIMEOUT * 1000);
    connect(this->m_waitingAppsTimeoutID, SIGNAL(timeout()), this, SLOT(onPhaseStartupTimeout()));

    connect(this->m_appManager, SIGNAL(AppExited(App *)), this, SLOT(onAppExited(App *)));
    connect(this->m_clientManager, SIGNAL(clientAdded(Client *)), this, SLOT(onClientAdded(Client *)));
    connect(this->m_clientManager, SIGNAL(clientChanged(Client *)), this, SLOT(onClientChanged(Client *)));
    connect(this->m_clientManager, SIGNAL(clientDeleted(Client *)), this, SLOT(onClientDeleted(Client *)));
    connect(this->m_clientManager, SIGNAL(interactRequesting(Client *)), this, SLOT(onInteractRequest(Client *)));
    connect(this->m_clientManager, SIGNAL(interactDone(Client *)), this, SLOT(onInteractDone(Client *)));
    connect(this->m_clientManager, SIGNAL(shutdownCanceled(Client *)), this, SLOT(onShutdownCanceled(Client *)));
    connect(this->m_clientManager, SIGNAL(endSessionPhase2Requesting(Client *)), this, SLOT(onEndSessionPhase2Request(Client *)));
    connect(this->m_clientManager, SIGNAL(endSessionResponse(Client *)), this, SLOT(onEndSessionResponse(Client *)));

    connect(this->m_inhibitorManager, SIGNAL(inhibitorAdded(QSharedPointer<Inhibitor>)), this, SLOT(onInhibitorAdded(QSharedPointer<Inhibitor>)));
    connect(this->m_inhibitorManager, SIGNAL(inhibitorDeleted(QSharedPointer<Inhibitor>)), this, SLOT(onInhibitorDeleted(QSharedPointer<Inhibitor>)));

    auto sessionConnection = QDBusConnection::sessionBus();
    if (!sessionConnection.registerService(KSM_DBUS_NAME))
    {
        KLOG_WARNING() << "Failed to register dbus name: " << KSM_DBUS_NAME;
    }

    if (!sessionConnection.registerObject(KSM_DBUS_OBJECT_PATH, this))
    {
        KLOG_WARNING() << "Can't register object:" << sessionConnection.lastError();
    }
}

void SessionManager::processPhase()
{
    KLOG_DEBUG() << "Start phase: " << App::phaseEnum2str(this->m_currentPhase);

    this->m_waitingApps.clear();
    this->m_waitingAppsTimeoutID->stop();
    this->m_waitingClients.clear();
    this->m_waitingClientsTimeoutID->disconnect();
    this->m_waitingClientsTimeoutID->stop();

    Q_EMIT this->PhaseChanged(this->m_currentPhase);

    switch (this->m_currentPhase)
    {
    case KSMPhase::KSM_PHASE_DISPLAY_SERVER:
    case KSMPhase::KSM_PHASE_POST_DISPLAY_SERVER:
    case KSMPhase::KSM_PHASE_INITIALIZATION:
    case KSMPhase::KSM_PHASE_WINDOW_MANAGER:
    case KSMPhase::KSM_PHASE_PANEL:
    case KSMPhase::KSM_PHASE_DESKTOP:
    case KSMPhase::KSM_PHASE_APPLICATION:
        this->processPhaseStartup();
        break;
    case KSMPhase::KSM_PHASE_RUNNING:
        break;
    case KSMPhase::KSM_PHASE_QUERY_END_SESSION:
        this->processPhaseQueryEndSession();
        break;
    case KSMPhase::KSM_PHASE_END_SESSION_PHASE1:
        this->processPhaseEndSessionPhase1();
        break;
    case KSMPhase::KSM_PHASE_END_SESSION_PHASE2:
        this->processPhaseEndSessionPhase2();
        break;
    case KSMPhase::KSM_PHASE_EXIT:
        this->processPhaseExit();
        break;
    default:
        break;
    }
}

void SessionManager::processPhaseStartup()
{
    auto apps = this->m_appManager->startApps(this->m_currentPhase);

    // 在KSM_PHASE_APPLICATION阶段前启动的应用在运行后需要（通过dbus或者xsmp规范）告知会话管理自己已经启动成功，这样会话管理才会进入下一个阶段。
    if (this->m_currentPhase < KSMPhase::KSM_PHASE_APPLICATION)
    {
        this->m_waitingApps = std::move(apps);
    }

    // 一些应用需要延时执行或者需要等待其启动完毕的信号
    if (this->m_waitingApps.size() > 0)
    {
        if (this->m_currentPhase < KSMPhase::KSM_PHASE_APPLICATION)
        {
            this->m_waitingAppsTimeoutID->start();
        }
    }
    else
    {
        this->startNextPhase();
    }
}

void SessionManager::processPhaseQueryEndSession()
{
    for (auto client : this->m_clientManager->getClients())
    {
        if (!client->queryEndSession(true))
        {
            KLOG_WARNING() << "Failed to query client:" << client->getID();
        }
        else
        {
            this->m_waitingClients.push_back(client);
        }
    }
    if (this->m_waitingClients.size() > 0)
    {
        connect(this->m_waitingClientsTimeoutID,
                &QTimer::timeout, [this]() -> void
                { this->onWaitingSessionTimeout(std::bind(&SessionManager::queryEndSessionComplete, this)); });

        this->m_waitingClientsTimeoutID->start(300);
    }
    else
    {
        this->startNextPhase();
    }
}

void SessionManager::queryEndSessionComplete()
{
    KLOG_DEBUG() << "Query end session complete.";

    if (this->m_currentPhase != KSMPhase::KSM_PHASE_QUERY_END_SESSION)
    {
        KLOG_WARNING() << "The phase is error. phase: " << App::phaseEnum2str(this->m_currentPhase);
        return;
    }

    this->m_waitingClients.clear();
    this->m_waitingClientsTimeoutID->disconnect();
    this->m_waitingClientsTimeoutID->stop();

    if (this->m_process->state() != QProcess::Running)
    {
        this->m_process->setProgram("/usr/bin/kiran-session-window");
        this->m_process->setArguments(QStringList(QString("--power-action=%1").arg(this->m_powerAction)));
        this->m_process->start();
        connect(this->m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onExitWindowResponse()));
    }
}

void SessionManager::cancelEndSession()
{
    RETURN_IF_TRUE(this->m_currentPhase < KSMPhase::KSM_PHASE_QUERY_END_SESSION);

    // 如果取消退出，则之前通过Interact Request请求加入的抑制器需要全部移除
    this->m_inhibitorManager->deleteInhibitorsWithStartupID();

    for (auto client : this->m_clientManager->getClients())
    {
        if (!client->cancelEndSession())
        {
            KLOG_WARNING() << "Failed to cancel end session for the client: " << client->getID();
        }
    }

    // this->exit_query_window_ = nullptr;
    this->m_powerAction = PowerAction::POWER_ACTION_NONE;

    // 回到运行阶段
    this->m_currentPhase = KSMPhase::KSM_PHASE_RUNNING;
    // 不能在process_phase中清理第二阶段缓存客户端，因为在调用process_phase_end_session_phase2函数时需要使用
    this->phase2_request_clients_.clear();
    this->processPhase();
}

void SessionManager::processPhaseEndSessionPhase1()
{
    for (auto client : this->m_clientManager->getClients())
    {
        if (!client->endSession(false))
        {
            KLOG_WARNING() << "Failed to end client: " << client->getID();
        }
        else
        {
            this->m_waitingClients.push_back(client);
        }
    }

    if (this->m_waitingClients.size() > 0)
    {
        connect(this->m_waitingClientsTimeoutID,
                &QTimer::timeout, [this]() -> void
                { this->onWaitingSessionTimeout(std::bind(&SessionManager::startNextPhase, this)); });
        this->m_waitingClientsTimeoutID->start(KSM_PHASE_STARTUP_TIMEOUT * 1000);
    }
    else
    {
        this->startNextPhase();
    }
}

void SessionManager::processPhaseEndSessionPhase2()
{
    // 如果有客户端需要进行第二阶段的数据保存操作，则告知这些客户端现在可以开始进行了
    if (this->phase2_request_clients_.size() > 0)
    {
        for (auto client : this->phase2_request_clients_)
        {
            if (!client->endSessionPhase2())
            {
                KLOG_WARNING() << "Failed to end session phase2 for the client: " << client->getID();
            }
            else
            {
                this->m_waitingClients.push_back(client);
            }
        }
        this->phase2_request_clients_.clear();
    }

    if (this->m_waitingClients.size() > 0)
    {
        connect(this->m_waitingClientsTimeoutID,
                &QTimer::timeout, [this]() -> void
                { this->onWaitingSessionTimeout(std::bind(&SessionManager::startNextPhase, this)); });

        this->m_waitingClientsTimeoutID->start(KSM_PHASE_STARTUP_TIMEOUT * 1000);
    }
    else
    {
        this->startNextPhase();
    }
}

void SessionManager::processPhaseExit()
{
    for (auto client : this->m_clientManager->getClients())
    {
        if (!client->stop())
        {
            KLOG_WARNING() << "Failed to stop client: " << client->getID();
        }
    }

    // FIXBUG: #53714
    this->maybeRestartUserBus();
    this->startNextPhase();
}

void SessionManager::maybeRestartUserBus()
{
    RETURN_IF_TRUE(!SystemdLogin1::getDefault()->isLastSession());

    KLOG_DEBUG() << "Restart dbus.service.";

    auto sendMessage = QDBusMessage::createMethodCall(SYSTEMD_DBUS_NAME,
                                                      SYSTEMD_DBUS_OBJECT_PATH,
                                                      SYSTEMD_DBUS_INTERFACE_NAME,
                                                      "TryRestartUnit");
    sendMessage << QString("dbus.service") << QString("replace");
    auto replyMessage = QDBusConnection::sessionBus().call(sendMessage, QDBus::Block, DBUS_TIMEOUT_MS);

    if (replyMessage.type() == QDBusMessage::ErrorMessage)
    {
        KLOG_WARNING() << "Call TryRestartUnit failed: " << replyMessage.errorMessage();
    }
}

void SessionManager::startNextPhase()
{
    bool start_next_phase = true;

    switch (this->m_currentPhase)
    {
    case KSMPhase::KSM_PHASE_IDLE:
    case KSMPhase::KSM_PHASE_DISPLAY_SERVER:
    case KSMPhase::KSM_PHASE_POST_DISPLAY_SERVER:
    case KSMPhase::KSM_PHASE_INITIALIZATION:
    case KSMPhase::KSM_PHASE_WINDOW_MANAGER:
    case KSMPhase::KSM_PHASE_PANEL:
    case KSMPhase::KSM_PHASE_DESKTOP:
        break;
    case KSMPhase::KSM_PHASE_APPLICATION:
        this->processPhaseApplicationEnd();
        break;
    case KSMPhase::KSM_PHASE_RUNNING:
        break;
    case KSMPhase::KSM_PHASE_QUERY_END_SESSION:
        break;
    case KSMPhase::KSM_PHASE_END_SESSION_PHASE1:
        // if (auto_save_is_enabled(manager))
        //     maybe_save_session(manager);
        break;
    case KSMPhase::KSM_PHASE_EXIT:
        this->quitSession();
        start_next_phase = false;
        break;
    default:
        break;
    }

    if (start_next_phase)
    {
        this->m_currentPhase = (KSMPhase)(this->m_currentPhase + 1);
        KLOG_DEBUG() << "Start next phase: " << App::phaseEnum2str(this->m_currentPhase);
        this->processPhase();
    }
    else
    {
        KLOG_DEBUG() << "Keep current phase: " << App::phaseEnum2str(this->m_currentPhase);
    }
}

void SessionManager::processPhaseApplicationEnd()
{
    connect(SignalHandler::get_default(),
            &SignalHandler::signalReceived,
            QCoreApplication::instance(),
            std::bind(&SessionManager::onSystemSignal, this, std::placeholders::_1));
    /* 需要监听SIGTERM信号，否则在maybeRestartUserBus函数中重置dbus.service时会发送SIGTERM信号，
       导致程序直接退出进入登录界面，如果此时是重启请求，则重启功能失效。*/
    SignalHandler::get_default()->addSignal(SIGTERM);
}

void SessionManager::quitSession()
{
    KLOG_DEBUG("Quit Session.");

    this->m_power->doPowerAction(this->m_powerAction);

    QCoreApplication::instance()->quit();
}

void SessionManager::onAppStartupFinished(App *app)
{
    RETURN_IF_FALSE(app);

    auto iter = std::remove_if(this->m_waitingApps.begin(),
                               this->m_waitingApps.end(),
                               [app](App *item) -> bool
                               {
                                   return item->getAppID() == app->getAppID();
                               });

    this->m_waitingApps.erase(iter, this->m_waitingApps.end());

    if (this->m_waitingApps.size() == 0 &&
        this->m_currentPhase < KSMPhase::KSM_PHASE_APPLICATION)
    {
        if (this->m_waitingAppsTimeoutID->isActive())
        {
            this->m_waitingAppsTimeoutID->stop();
        }
        this->startNextPhase();
    }
}

}  // namespace Kiran
