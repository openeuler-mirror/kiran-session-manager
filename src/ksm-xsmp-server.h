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

#include <X11/ICE/ICEconn.h>
#include <X11/ICE/ICElib.h>
#include <X11/ICE/ICEutil.h>
#include <X11/SM/SMlib.h>
#include "lib/base.h"

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

namespace Kiran
{
struct KSMConnectionWatch;

class KSMXsmpServer
{
public:
    KSMXsmpServer();
    virtual ~KSMXsmpServer();

    static KSMXsmpServer *get_instance() { return instance_; };

    static void global_init();

    static void global_deinit() { delete instance_; };

    // 存在新的客户端连接
    sigc::signal<void, unsigned long *, SmsCallbacks *> signal_new_client_connected() { return this->new_client_connected_; };
    // 收到Ice连接错误或关闭的消息
    sigc::signal<void, IceProcessMessagesStatus, IceConn> signal_ice_conn_status_changed() { return this->ice_conn_status_changed_; };

private:
    void init();
    // 监听网络连接
    void listen_socket();
    // 更新认证文件
    void update_ice_authority();
    // 创建并通过IceSetPaAuthData将认证项存储在内存中
    IceAuthFileEntry *create_and_store_auth_entry(const std::string &protocol_name,
                                                  const std::string &network_id);

    static void on_sms_error_handler_cb(SmsConn sms_conn,
                                        Bool swap,
                                        int offending_minor_opcode,
                                        unsigned long offending_sequence_num,
                                        int error_class,
                                        int severity,
                                        IcePointer values);

    static void on_ice_io_error_handler_cb(IceConn ice_connection);

    static Status on_new_client_connection_cb(SmsConn sms_conn,
                                              SmPointer manager_data,
                                              unsigned long *mask_ret,
                                              SmsCallbacks *callbacks_ret,
                                              char **failure_reason_ret);

    static gboolean on_accept_ice_connection_cb(GIOChannel *source,
                                                GIOCondition condition,
                                                gpointer data);

    static bool on_ice_protocol_timeout_cb(IceConn ice_conn);
    static gboolean on_auth_iochannel_watch_cb(GIOChannel *source, GIOCondition condition, gpointer user_data);
    static void disconnect_ice_connection(IceConn ice_conn);
    static void free_connection_watch(KSMConnectionWatch *data);

private:
    static KSMXsmpServer *instance_;

    // 监听的socket总量
    int num_listen_sockets_;
    // 监听的本地socket数量
    int num_local_listen_sockets_;
    IceListenObj *listen_sockets_;

    sigc::signal<void, unsigned long *, SmsCallbacks *> new_client_connected_;
    sigc::signal<void, IceProcessMessagesStatus, IceConn> ice_conn_status_changed_;
};
}  // namespace Kiran
