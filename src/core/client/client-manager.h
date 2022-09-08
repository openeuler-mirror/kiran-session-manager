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

#include <QMap>
#include <QObject>
#include "src/core/client/client-dbus.h"
#include "src/core/client/client-xsmp.h"

struct _SmsConn;
typedef struct _SmsConn *SmsConn;
struct _IceConn;
typedef struct _IceConn *IceConn;

class QDBusServiceWatcher;

namespace Kiran
{
/*
基于XSMP协议的交互逻辑：

1. 请求会话结束阶段（query end session)：
  1.1 服务器向所有客户端调用SmsSaveYourself。
  1.2 如果SmsSaveYourself携带的interact_style参数是允许与用户交互，则：
    1.2.1 客户端可以调用SmsInteractRequest告知会话管理客户准备与用户交互，服务端回复SmsInteract表示同意，
         此时客户端可以通过弹出对话框等方式与用户交互，交互结束后调用SmcInteractDone告知会话管理交互已经结束。
    1.2.2 如果SmcInteractDone中携带的cancel_shutdown是True，则会话管理调用SmsShutdownCancelled告知所有客户端取消结束会话。
  1.3 客户端需要调用SmcSaveYourselfDone来响应前面的SmsSaveYourself，参数success按照如下情况设置：
    1.3.1 如果此时收到了取消结束会话的消息，那客户端可以选择不保存数据，然后将参数success设为False；
    1.3.2 客户端选择保存数据，数据保存成功后将success设置为True，否则设置为False。
  1.4 当收到所有客户端的"Save Yourself Done"消息后，且会话为被取消，则进入会话结束阶段。
2.会话结束阶段 （end session）：
  2.1 服务器向所有客户端调用SmsSaveYourself，参数interact_style设置为False不再允许客户端与用户交互。
    2.1.1 对于普通的客户端，在保存好数据后调用SmcSaveYourselfDone函数即可。
    2.1.2 对于部分特殊客户端（例如窗口管理器）希望在其他客户端结束后再进行一些操作时，则进行如下处理：
      2.1.2.1 这些客户端应该调用SmcRequestSaveYourselfPhase2函数请求会话管理在其他客户端都执行完SmcSaveYourselfDone后再进行第二阶段的数据保存，
              随后特殊客户端应该进行数据保存操作，然后调用SmcSaveYourselfDone函数来响应会话管理的第一次"Save Yourself"消息。
      2.1.2.2 当收到所有客户端第一阶段的"Save Yourself Done"消息后，会话管理会向特殊客户端调用SmsSaveYourselfPhase2。
      2.1.2.3 特殊客户端在进行数据保存和一系列操作后，调用SmcSaveYourselfDone函数来响应会话管理的第二次"Save Yourself"消息。
  2.2 当收到所有客户端的第一阶段和第二阶段的"Save Yourself Done"消息后，会话管理进入真正的退出阶段。
3. 会话管理退出阶段 (quit)
    进程退出
*/

class XsmpServer;

class ClientManager : public QObject
{
    Q_OBJECT
public:
    ClientManager(XsmpServer *xsmp_server);
    virtual ~ClientManager(){};

    static ClientManager *getInstance() { return m_instance; };

    static void globalInit(XsmpServer *xsmp_server);

    static void globalDeinit() { delete m_instance; };

    Client *getClient(const QString &startupID) { return this->m_clients.value(startupID, nullptr); }
    ClientDBus *getClientByDBusName(const QString &dbusName);
    QList<Client *> getClients() { return this->m_clients.values(); }

    // 添加客户端
    ClientXsmp *addClientXsmp(const QString &startupID, SmsConn smsConn);
    ClientDBus *addClientDBus(const QString &startupID, const QString &dbusName, const QString &appID);

    // 删除客户端
    bool deleteClient(const QString &startupID);

    // 通过sms连接获取对应的KSMClient对象
    ClientXsmp *getClientBySmsConn(SmsConn smsConn);
    ClientXsmp *getClientByIceConn(IceConn iceConn);

Q_SIGNALS:
    // 客户端被添加信号
    void clientAdded(Client *client);
    // 客户端属性变化
    void clientChanged(Client *client);
    // 客户端被删除信号
    void clientDeleted(Client *client);
    // 客户端请求与用户进行交互
    void interactRequesting(Client *client);
    // 客户端与用户交互完毕
    void interactDone(Client *client);
    // 客户端申请退出会话
    void shutdownRequesting(Client *client);
    // 客户端申请取消退出会话
    void shutdownCanceled(Client *client);
    // 客户端请求"Save Yourself Phase2"
    void endSessionPhase2Requesting(Client *client);
    // 客户端响应退出会话请求，这里无法区分是响应的第一阶段请求还是第二阶段，所以应该在请求的时候自己缓存状态
    void endSessionResponse(Client *client);

private Q_SLOTS:
    void onNameLost(const QString &dbusName);

private:
    void init();
    // 添加客户端
    bool addClient(Client *client);

private slots:
    void onNewXsmpClientConnected(unsigned long *maskRet, void *callbacks);
    void onIceConnStatusChanged(int32_t status, IceConn iceConn);
    void onDBusClientEndSessionResponse(bool isOK, QString startupID);

private:
    static ClientManager *m_instance;
    XsmpServer *m_xsmpServer;

    QDBusServiceWatcher *m_serviceWatcher;

    // 维护客户端对象 <startup_id, KSMClient>
    QMap<QString, Client *> m_clients;
};
}  // namespace Kiran
