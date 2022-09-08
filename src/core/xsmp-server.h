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

#include <glib.h>
#include <QObject>

/* 
XSMP是一套X会话管理的规范，会话管理和客户端之间的通信是遵循ICE协议规范。ICE协议规范定义了一套客户端之间的交互接口。

ICE通信的大致过程如下：
1. 初始化
  1.1 客户端：
    1.1.1 客户端需要调用IceRegisterForProtocolSetup函数进行协议注册，该函数返回一个主协议号。
    1.1.2 客户端调用IceOpenConnection函数与服务器进行连接，获取连接的句柄
    1.1.3 客户端调用IceProtocolSetup函数并附带主协议号来安装（激活）之前注册的协议
  1.2 服务端：
    1.2.1 服务端调用IceRegisterForProtocolReply来响应协议的激活
    1.2.2 服务端调用IceListenForConnections函数来监听客户端的连接
    1.2.3 当收到连接请求时，调用IceAcceptConnection获取连接句柄。
2. 连接成功
  2.1 客户端和服务器分别监听连接句柄，当句柄有可读数据时，可调用IceProcessMessages接口获取数据头信息，
      调用PoProcessMsgProc/IcePaProcessMsgProc获取消息体信息并进行unpack操作
*/

struct _IceConn;
typedef struct _IceConn *IceConn;
struct _IceListenObj;
typedef struct _IceListenObj *IceListenObj;

namespace Kiran
{
struct ConnectionWatch;

class XsmpServer : public QObject
{
    Q_OBJECT
public:
    XsmpServer();
    virtual ~XsmpServer();

    static XsmpServer *getInstance() { return m_instance; };

    static void globalInit();

    static void globalDeinit() { delete m_instance; };

signals:
    // 存在新的客户端连接
    void newClientConnected(unsigned long *, void *);
    // 收到Ice连接错误或关闭的消息
    void iceConnStatusChanged(int32_t, IceConn);

private:
    void init();
    // 监听网络连接
    void listenSocket();
    // 更新认证文件
    void updateIceAuthority();
    // 创建并通过IceSetPaAuthData将认证项存储在内存中
    void *createAndStoreAuthEntry(const std::string &protocolName,
                                  const std::string &networkID);

    static gboolean onAcceptIceConnection(GIOChannel *source,
                                          GIOCondition condition,
                                          gpointer data);

    static gboolean onAuthIochannelWatch(GIOChannel *source, GIOCondition condition, gpointer user_data);

private:
    static XsmpServer *m_instance;

    // 监听的socket总量
    int m_numListenSockets;
    // 监听的本地socket数量
    int m_numLocalListenSockets;
    IceListenObj *m_listenSockets;
};
}  // namespace Kiran
