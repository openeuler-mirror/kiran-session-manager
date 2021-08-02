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

#include <X11/SM/SMlib.h>
#include "src/client/ksm-client-dbus.h"
#include "src/client/ksm-client-xsmp.h"
#include "src/ksm-xsmp-server.h"

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

class KSMClientXsmp;

class KSMClientManager
{
public:
    KSMClientManager(KSMXsmpServer *xsmp_server);
    virtual ~KSMClientManager(){};

    static KSMClientManager *get_instance() { return instance_; };

    static void global_init(KSMXsmpServer *xsmp_server);

    static void global_deinit() { delete instance_; };

    std::shared_ptr<KSMClient> get_client(const std::string &startup_id) { return MapHelper::get_value(this->clients_, startup_id); }
    KSMClientVec get_clients() { return MapHelper::get_values(this->clients_); };

    // 添加客户端
    std::shared_ptr<KSMClientXsmp> add_client_xsmp(SmsConn sms_conn, const std::string &startup_id);
    std::shared_ptr<KSMClientDBus> add_client_dbus(const std::string &startup_id);

    // 删除客户端
    bool delete_client(const std::string &startup_id);

    // 客户端被添加/删除信号
    sigc::signal<void, std::shared_ptr<KSMClient>> signal_client_added() { return this->client_added_; };
    sigc::signal<void, std::shared_ptr<KSMClient>> signal_client_deleted() { return this->client_deleted_; };
    // 客户端申请退出会话
    sigc::signal<void, std::shared_ptr<KSMClient>> signal_shutdown_request() { return this->shutdown_request_; };
    // 客户端申请取消退出会话
    sigc::signal<void, std::shared_ptr<KSMClient>> signal_shutdown_canceled() { return this->shutdown_canceled_; };
    // 客户端请求"Save Yourself Phase2"
    sigc::signal<void, std::shared_ptr<KSMClient>> signal_end_session_phase2_request() { return this->end_session_phase2_request_; };
    // 客户端响应退出会话请求，这里无法区分是响应的第一阶段请求还是第二阶段，所以应该在请求的时候自己缓存状态
    sigc::signal<void, std::shared_ptr<KSMClient>> signal_end_session_response() { return this->end_session_response_; };

private:
    void init();

    // 添加客户端
    bool add_client(std::shared_ptr<KSMClient> client);
    // 通过sms连接获取对应的KSMClient对象
    std::shared_ptr<KSMClientXsmp> get_client_by_sms_conn(SmsConn sms_conn);
    std::shared_ptr<KSMClientXsmp> get_client_by_ice_conn(IceConn ice_conn);

    void on_new_xsmp_client_connected_cb(unsigned long *mask_ret, SmsCallbacks *callbacks_ret);
    void on_ice_conn_status_changed_cb(IceProcessMessagesStatus status, IceConn ice_conn);
    void on_dbus_client_end_session_response(bool is_ok, std::shared_ptr<KSMClient> client);

    static Status on_register_client_cb(SmsConn sms_conn, SmPointer manager_data, char *previous_id);
    static void on_interact_request_cb(SmsConn sms_conn, SmPointer manager_data, int dialog_type);
    static void on_interact_done_cb(SmsConn sms_conn, SmPointer manager_data, Bool cancel_shutdown);
    static void on_save_yourself_request_cb(SmsConn sms_conn,
                                            SmPointer manager_data,
                                            int save_type,
                                            Bool shutdown,
                                            int interact_style,
                                            Bool fast,
                                            Bool global);
    static void on_save_yourself_phase2_request_cb(SmsConn sms_conn, SmPointer manager_data);
    static void on_save_yourself_done_cb(SmsConn sms_conn, SmPointer manager_data, Bool success);
    static void on_close_connection_cb(SmsConn sms_conn,
                                       SmPointer manager_data,
                                       int count,
                                       char **reason_msgs);
    static void on_set_properties_cb(SmsConn sms_conn,
                                     SmPointer manager_data,
                                     int num_props,
                                     SmProp **props);
    static void on_delete_properties_cb(SmsConn sms_conn,
                                        SmPointer manager_data,
                                        int num_props,
                                        char **prop_names);
    static void on_get_properties_cb(SmsConn sms_conn, SmPointer manager_data);

private:
    static KSMClientManager *instance_;
    KSMXsmpServer *xsmp_server_;

    // 维护客户端对象 <startup_id, KSMClient>
    std::map<std::string, std::shared_ptr<KSMClient>> clients_;

    sigc::signal<void, std::shared_ptr<KSMClient>> client_added_;
    sigc::signal<void, std::shared_ptr<KSMClient>> client_deleted_;

    sigc::signal<void, std::shared_ptr<KSMClient>> shutdown_request_;
    sigc::signal<void, std::shared_ptr<KSMClient>> shutdown_canceled_;
    sigc::signal<void, std::shared_ptr<KSMClient>> end_session_phase2_request_;
    sigc::signal<void, std::shared_ptr<KSMClient>> end_session_response_;
};
}  // namespace Kiran
