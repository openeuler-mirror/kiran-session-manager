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

#include "src/core/client/client-manager.h"

#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include "lib/base/base.h"
#include "src/core/client/client-dbus.h"
#include "src/core/client/client-xsmp.h"
#include "src/core/utils.h"
#include "src/core/xsmp-server.h"

#include <X11/ICE/ICEconn.h>
#include <X11/ICE/ICElib.h>
#include <X11/ICE/ICEutil.h>
#include <X11/SM/SMlib.h>

namespace Kiran
{
static Status onRegisterClient(SmsConn smsConn, SmPointer managerData, char *previousID)
{
    KLOG_DEBUG() << "Receive register client message.";

    /* SMS协议规范：
       1. 如果应用程序第一次加入会话，则previouse_id为NULL，这时需要会话管理负责生成一个唯一的id，
       如果应用程序根据上一次的会话重启，则previouse_id记录的是上一次会话管理生成的id。
       2. 会话管理需要通过SmsRegisterClientReply函数告知客户端注册已经成功，
          2.1 如果previous_id不为NULL，则携带的client_id参数应该跟previous_id相同；
          2.2 如果previous_id为NULL，则使用会话管理生成的唯一id并需要向客户端发送"Save Yourself"消息，
          该消息携带的参数为type=Local, shutdown=False,interact-style=None, fast=False。*/

    auto clientManager = static_cast<ClientManager *>(managerData);
    auto newPreviousID = POINTER_TO_STRING(previousID);

    SCOPE_EXIT(
        {
            if (previousID)
            {
                free(previousID);
            }
        });

    auto client = clientManager->addClientXsmp(POINTER_TO_STRING(previousID), smsConn);
    RETURN_VAL_IF_FALSE(client, False);

    KLOG_DEBUG() << "Startup id: " << client->getID() << ", previous_id: " << POINTER_TO_STRING(previousID);

    auto clientID = client->getID().toUtf8();
    SmsRegisterClientReply(smsConn, clientID.data());
    if (!previousID)
    {
        SmsSaveYourself(smsConn, SmSaveLocal, False, SmInteractStyleNone, False);
    }
    return True;
}

static void onInteractRequest(SmsConn smsConn, SmPointer managerData, int dialogType)
{
    KLOG_DEBUG() << "Receive interact request message.";

    auto clientManager = static_cast<ClientManager *>(managerData);
    auto client = clientManager->getClientBySmsConn(smsConn);
    RETURN_IF_FALSE(client);

    Q_EMIT clientManager->interactRequesting(client);

    KLOG_DEBUG() << "Startup id: " << client->getID() << ", dialog type: " << dialogType;
    SmsInteract(smsConn);
}

static void onInteractDone(SmsConn smsConn, SmPointer managerData, Bool cancelShutdown)
{
    KLOG_DEBUG() << "Receive interact done message.";

    auto clientManager = static_cast<ClientManager *>(managerData);
    auto client = clientManager->getClientBySmsConn(smsConn);
    RETURN_IF_FALSE(client);

    KLOG_DEBUG() << "Startup id: " << client->getID() << ", cancel shutdown: " << cancelShutdown;
    if (cancelShutdown)
    {
        Q_EMIT clientManager->shutdownCanceled(client);
    }

    /* 取消关闭(shutdown_canceled)信号应该要先于interact_done信号，否则如果先收到interact_done信号的话，
       SessinManager会移除交互时添加的抑制器，当没有抑制器时，SessinManager会进入下一个阶段。此时再收到取消关闭信号
       可能回被SessinManager忽略。*/
    Q_EMIT clientManager->interactDone(client);
}

static void onSaveYourselfRequest(SmsConn smsConn, SmPointer managerData, int saveType, Bool shutdown, int interactStyle, Bool fast, Bool global)
{
    KLOG_DEBUG() << "Receive save yourself request message.";
    /* 如果global为False，则会话管理应该只向请求的客户端发送"Save Yourself"消息；如果global为True，
       则会话管理应该向所有客户端发送"Save Yourself"消息。当global为True时，我们只在shutdown为True
       的情况向所有客户端发送"Save Yourself"消息，对应的shutdown设置为True，其他情况忽略处理*/

    auto clientManager = static_cast<ClientManager *>(managerData);
    auto client = clientManager->getClientBySmsConn(smsConn);
    RETURN_IF_FALSE(client);

    KLOG_DEBUG() << "Startup id: " << client->getID() << ", save type: "
                 << saveType << ", shutdown: " << shutdown << ", interact style: "
                 << interactStyle << ", fast: " << fast << ", global: " << global;

    if (shutdown && global)
    {
        Q_EMIT clientManager->shutdownRequesting(client);
    }
    else if (!shutdown && !global)
    {
        SmsSaveYourself(smsConn, SmSaveLocal, False, SmInteractStyleAny, False);
    }
}

static void onSaveYourselfPhase2Request(SmsConn smsConn, SmPointer managerData)
{
    KLOG_DEBUG() << "Receive save yourself phase2 request message.";

    auto clientManager = static_cast<ClientManager *>(managerData);
    auto client = clientManager->getClientBySmsConn(smsConn);
    RETURN_IF_FALSE(client);
    Q_EMIT clientManager->endSessionPhase2Requesting(client);
}

static void onSaveYourselfDone(SmsConn smsConn, SmPointer managerData, Bool success)
{
    auto clientManager = static_cast<ClientManager *>(managerData);
    auto client = clientManager->getClientBySmsConn(smsConn);
    RETURN_IF_FALSE(client);

    KLOG_DEBUG() << "Receive save yourself done message. Startup id: " << client->getID() << ", success: " << success;

    SmsSaveComplete(smsConn);
    Q_EMIT clientManager->endSessionResponse(client);
}

static void onCloseConnection(SmsConn smsConn,
                              SmPointer managerData,
                              int count,
                              char **reasonMsgs)
{
    auto clientManager = static_cast<ClientManager *>(managerData);
    auto client = clientManager->getClientBySmsConn(smsConn);
    RETURN_IF_FALSE(client);

    KLOG_DEBUG() << "Receive client " << client->getID() << " closed connection message.";

    for (int32_t i = 0; i < count; i++)
    {
        KLOG_DEBUG() << "Close reason: " << reasonMsgs[i];
    }
    SmFreeReasons(count, reasonMsgs);
    clientManager->deleteClient(client->getID());
}

static void onSetProperties(SmsConn smsConn,
                            SmPointer managerData,
                            int numProps,
                            SmProp **props)
{
    auto clientManager = static_cast<ClientManager *>(managerData);
    auto client = clientManager->getClientBySmsConn(smsConn);
    RETURN_IF_FALSE(client);

    for (int32_t i = 0; i < numProps; ++i)
    {
        client->updateProperty(props[i]);
    }
    free(props);

    Q_EMIT clientManager->clientChanged(client);
}

static void onDeleteProperties(SmsConn smsConn,
                               SmPointer managerData,
                               int numProps,
                               char **propNames)
{
    auto clientManager = static_cast<ClientManager *>(managerData);
    auto client = clientManager->getClientBySmsConn(smsConn);
    RETURN_IF_FALSE(client);

    for (int32_t i = 0; i < numProps; ++i)
    {
        client->deleteProperty(propNames[i]);
    }
    // 文档中没有提到需要释放该变量
    free(propNames);

    Q_EMIT clientManager->clientChanged(client);
}

static void onGetProperties(SmsConn smsConn, SmPointer managerData)
{
    auto clientManager = static_cast<ClientManager *>(managerData);
    auto client = clientManager->getClientBySmsConn(smsConn);
    RETURN_IF_FALSE(client);

    auto props = client->get_properties();
    SmsReturnProperties(smsConn, props.length(), reinterpret_cast<SmProp **>(props.data()));
}

ClientManager::ClientManager(XsmpServer *xsmp_server) : m_xsmpServer(xsmp_server)
{
    this->m_serviceWatcher = new QDBusServiceWatcher(this);
}

ClientManager *ClientManager::m_instance = nullptr;
void ClientManager::globalInit(XsmpServer *xsmp_server)
{
    m_instance = new ClientManager(xsmp_server);
    m_instance->init();
}

ClientDBus *ClientManager::getClientByDBusName(const QString &dbusName)
{
    for (auto iter : this->m_clients)
    {
        CONTINUE_IF_TRUE(iter->getType() != CLIENT_TYPE_DBUS);
        auto dbus_client = dynamic_cast<ClientDBus *>(iter);
        RETURN_VAL_IF_TRUE(dbus_client->getDBusName() == dbusName, dbus_client);
    }
    return nullptr;
}

void ClientManager::init()
{
    KLOG_DEBUG() << "Init client manager.";

    this->m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    this->m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);

    connect(this->m_serviceWatcher, SIGNAL(serviceUnregistered(const QString &)), this, SLOT(onNameLost(const QString &)));
    connect(this->m_xsmpServer, SIGNAL(newClientConnected(unsigned long *, void *)), this, SLOT(onNewXsmpClientConnected(unsigned long *, void *)));
    connect(this->m_xsmpServer, SIGNAL(iceConnStatusChanged(int32_t, IceConn)), this, SLOT(onIceConnStatusChanged(int32_t, IceConn)));
}

ClientXsmp *ClientManager::addClientXsmp(const QString &startupID, SmsConn smsConn)
{
    auto newStartupID = startupID;
    if (newStartupID.length() == 0)
    {
        newStartupID = Utils::getDefault()->generateStartupID();
    }

    auto client = new ClientXsmp(newStartupID, smsConn, this);

    if (!this->addClient(client))
    {
        delete client;
        return nullptr;
    }
    return client;
}

ClientDBus *ClientManager::addClientDBus(const QString &startupID, const QString &dbusName, const QString &appID)
{
    KLOG_DEBUG() << "Add dbus client. StartupID: " << startupID
                 << ", DBusName: " << dbusName << ", appID: " << appID;

    auto newStartupID = startupID;
    if (newStartupID.length() == 0)
    {
        newStartupID = Utils::getDefault()->generateStartupID();
    }

    auto client = new ClientDBus(newStartupID, dbusName, appID, this);

    if (!this->addClient(client))
    {
        delete client;
        return nullptr;
    }

    this->m_serviceWatcher->addWatchedService(dbusName);
    connect(client, &ClientDBus::endSessionResponse, this, std::bind(&ClientManager::onDBusClientEndSessionResponse, this, std::placeholders::_1, client->getID()));
    return client;
}

bool ClientManager::deleteClient(const QString &startupID)
{
    auto client = this->getClient(startupID);
    RETURN_VAL_IF_FALSE(client, false);

    this->m_clients.remove(startupID);
    Q_EMIT this->clientDeleted(client);
    return true;
}

bool ClientManager::addClient(Client *client)
{
    RETURN_VAL_IF_FALSE(client, false);

    if (this->m_clients.find(client->getID()) != this->m_clients.end())
    {
        KLOG_WARNING() << "The client " << client->getID() << " already exist.";
        return false;
    }
    else
    {
        this->m_clients.insert(client->getID(), client);
        Q_EMIT this->clientAdded(client);
    }

    return true;
}

ClientXsmp *ClientManager::getClientBySmsConn(SmsConn smsConn)
{
    auto id = SmsClientID(smsConn);
    auto client = this->getClient(POINTER_TO_STRING(id));

    if (!client)
    {
        KLOG_WARNING() << "The client isn't found. startup_id: " << POINTER_TO_STRING(id);
        return nullptr;
    }

    if (client->getType() != ClientType::CLIENT_TYPE_XSMP)
    {
        KLOG_WARNING() << "The client type isn't xsmp. startup_id: " << client->getID();
        return nullptr;
    }
    return (ClientXsmp *)client;
}

ClientXsmp *ClientManager::getClientByIceConn(IceConn iceConn)
{
    for (auto iter : this->m_clients)
    {
        CONTINUE_IF_TRUE(iter->getType() != ClientType::CLIENT_TYPE_XSMP);
        auto xsmp_client = static_cast<ClientXsmp *>(iter);

        if (SmsGetIceConnection(xsmp_client->get_sms_connection()) == iceConn)
        {
            return xsmp_client;
        }
    }
    return nullptr;
}

void ClientManager::onNameLost(const QString &dbusName)
{
    auto dbusClient = this->getClientByDBusName(dbusName);
    if (dbusClient)
    {
        KLOG_DEBUG() << "Remove client " << dbusClient->getID() << " which dbus name is " << dbusName;
        this->deleteClient(dbusClient->getID());
    }
}

void ClientManager::onNewXsmpClientConnected(unsigned long *mask_ret, void *callbacks)
{
    SmsCallbacks *callbacks_ret = static_cast<SmsCallbacks *>(callbacks);

    *mask_ret = 0;

    *mask_ret |= SmsRegisterClientProcMask;
    callbacks_ret->register_client.callback = onRegisterClient;
    callbacks_ret->register_client.manager_data = this;

    *mask_ret |= SmsInteractRequestProcMask;
    callbacks_ret->interact_request.callback = onInteractRequest;
    callbacks_ret->interact_request.manager_data = this;

    *mask_ret |= SmsInteractDoneProcMask;
    callbacks_ret->interact_done.callback = onInteractDone;
    callbacks_ret->interact_done.manager_data = this;

    *mask_ret |= SmsSaveYourselfRequestProcMask;
    callbacks_ret->save_yourself_request.callback = onSaveYourselfRequest;
    callbacks_ret->save_yourself_request.manager_data = this;

    *mask_ret |= SmsSaveYourselfP2RequestProcMask;
    callbacks_ret->save_yourself_phase2_request.callback = onSaveYourselfPhase2Request;
    callbacks_ret->save_yourself_phase2_request.manager_data = this;

    *mask_ret |= SmsSaveYourselfDoneProcMask;
    callbacks_ret->save_yourself_done.callback = onSaveYourselfDone;
    callbacks_ret->save_yourself_done.manager_data = this;

    *mask_ret |= SmsCloseConnectionProcMask;
    callbacks_ret->close_connection.callback = onCloseConnection;
    callbacks_ret->close_connection.manager_data = this;

    *mask_ret |= SmsSetPropertiesProcMask;
    callbacks_ret->set_properties.callback = onSetProperties;
    callbacks_ret->set_properties.manager_data = this;

    *mask_ret |= SmsDeletePropertiesProcMask;
    callbacks_ret->delete_properties.callback = onDeleteProperties;
    callbacks_ret->delete_properties.manager_data = this;

    *mask_ret |= SmsGetPropertiesProcMask;
    callbacks_ret->get_properties.callback = onGetProperties;
    callbacks_ret->get_properties.manager_data = this;
}

void ClientManager::onIceConnStatusChanged(int32_t status, IceConn iceConn)
{
    auto client = this->getClientByIceConn(iceConn);
    RETURN_IF_FALSE(client);

    switch (IceProcessMessagesStatus(status))
    {
    case IceProcessMessagesIOError:
        KLOG_WARNING() << "The client " << client->getID() << " Receive IceProcessMessagesIOError message. program name: " << client->getProgramName();
        this->deleteClient(client->getID());
        break;
    case IceProcessMessagesConnectionClosed:
        KLOG_DEBUG() << "The client " << client->getID() << " Receive IceProcessMessagesConnectionClosed message. program name: " << client->getProgramName();
        break;
    default:
        break;
    }
}

void ClientManager::onDBusClientEndSessionResponse(bool isOK, QString startupID)
{
    auto client = this->getClient(startupID);
    RETURN_IF_FALSE(client);

    if (isOK)
    {
        Q_EMIT this->endSessionResponse(client);
    }
    else
    {
        KLOG_WARNING() << "The Client " << client->getID() << " save failed.";
    }
}

}  // namespace Kiran
