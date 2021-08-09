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

#include <session_manager_dbus_stub.h>
#include "src/client/ksm-client.h"
#include "src/ksm-app.h"
#include "src/ksm-inhibitors.h"
#include "src/ksm-power.h"
#include "src/ksm-presence.h"
#include "src/ksm-quit-dialog.h"

namespace Kiran
{
class KSMAppManager;
class KSMClientManager;

class KSMManager : public gnome::SessionManagerStub
{
public:
    KSMManager(KSMAppManager *app_manager,
               KSMClientManager *client_manager);
    virtual ~KSMManager();

    static KSMManager *get_instance() { return instance_; };

    static void global_init(KSMAppManager *app_manager,
                            KSMClientManager *client_manager);

    static void global_deinit() { delete instance_; };

    // 会话开始
    void start();

    // 获取空闲超时管理器
    std::shared_ptr<KSMPresence> get_presence() { return this->presence_; };

protected:
    virtual void RegisterClient(const Glib::ustring &app_id,
                                const Glib::ustring &client_startup_id,
                                MethodInvocation &invocation);

    // 添加抑制器
    virtual void Inhibit(const Glib::ustring &app_id,
                         guint32 toplevel_xid,
                         const Glib::ustring &reason,
                         guint32 flags,
                         MethodInvocation &invocation);

    // 删除抑制器
    virtual void Uninhibit(guint32 inhibit_cookie,
                           MethodInvocation &invocation);

    // 判断指定flags的抑制器是否存在
    virtual void IsInhibited(guint32 flags,
                             MethodInvocation &invocation);

    // 获取所有抑制器
    virtual void GetInhibitors(MethodInvocation &invocation);

    // 关机
    virtual void Shutdown(MethodInvocation &invocation);
    virtual void RequestShutdown(MethodInvocation &invocation);
    virtual void CanShutdown(MethodInvocation &invocation);
    // 注销
    virtual void Logout(guint32 mode, MethodInvocation &invocation);
    virtual void CanLogout(MethodInvocation &invocation);
    // 重启
    virtual void Reboot(MethodInvocation &invocation);
    virtual void RequestReboot(MethodInvocation &invocation);
    virtual void CanReboot(MethodInvocation &invocation);

private:
    void init();

    void process_phase();
    // 开始阶段
    void process_phase_startup();
    // 准备结束会话，此时会询问已注册的客户端会话是否可以结束，客户端收到应该进行响应
    void process_phase_query_end_session();
    // 询问客户端会话是否结束处理完毕
    void query_end_session_complete();
    // 取消结束会话
    void cancel_end_session();
    // 会话结束第一阶段
    void process_phase_end_session_phase1();
    // 会话结束第二阶段
    void process_phase_end_session_phase2();
    // 退出阶段
    void process_phase_exit();

    // 开始下一个阶段
    void start_next_phase();

    // 所有跟客户端的交互都已经处理完毕，开始真正的退出会话
    void quit_session();
    // 触发登出动作
    void do_power_action(KSMPowerAction power_action);

    // KSMLogoutAction logout_type2action(KSMPowerAction logout_type);

    /* ----------------- 以下为回调处理函数 ------------------- */
    // 应用已启动完成回调
    void on_app_startup_finished(std::shared_ptr<KSMApp> app);
    // 应用启动超时
    bool on_phase_startup_timeout();
    bool on_waiting_session_timeout(std::function<void(void)> phase_complete_callback);
    // 退出会话确认对话框响应
    void on_quit_dialog_response(int32_t response_id);

    void on_app_exited_cb(std::shared_ptr<KSMApp> app);
    void on_client_added_cb(std::shared_ptr<KSMClient> client);
    void on_client_deleted_cb(std::shared_ptr<KSMClient> client);
    // xsmp客户端请求取消结束会话响应
    void on_xsmp_shutdown_canceled_cb(std::shared_ptr<KSMClient> client);
    void on_end_session_phase2_request_cb(std::shared_ptr<KSMClient> client);
    void on_end_session_response_cb(std::shared_ptr<KSMClient> client);

    void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name);
    void on_name_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name);
    void on_name_lost(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name);

private:
    static KSMManager *instance_;

    KSMAppManager *app_manager_;
    KSMClientManager *client_manager_;

    Glib::RefPtr<Gio::Settings> settings_;

    // 会话运行的阶段
    KSMPhase current_phase_;

    // 等待启动完成的应用
    KSMAppVec waiting_apps_;
    sigc::connection waiting_apps_timeout_id_;

    // 等待响应的客户端列表
    KSMClientVec waiting_clients_;
    sigc::connection waiting_clients_timeout_id_;
    // 请求进入第二阶段的客户端列表
    KSMClientVec phase2_request_clients_;

    KSMPower power_;
    // 抑制器管理
    KSMInhibitors inhibitors_;
    // 抑制对话框
    std::shared_ptr<KSMQuitDialog> quit_dialog_;
    // 空闲超时设置
    std::shared_ptr<KSMPresence> presence_;
    KSMPowerAction power_action_;

    uint32_t dbus_connect_id_;
    uint32_t object_register_id_;
};
}  // namespace Kiran