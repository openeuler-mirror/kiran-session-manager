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

#include "lib/base.h"

namespace Kiran
{
#define KSM_AUTOSTART_APP_ENABLED_KEY "X-KIRAN-Autostart-enabled"
#define KSM_AUTOSTART_APP_PHASE_KEY "X-KIRAN-Autostart-Phase"
//#define KSM_AUTOSTART_APP_AUTORESTART_KEY "X-KIRAN-AutoRestart"
#define KSM_AUTOSTART_APP_DELAY_KEY "X-KIRAN-Autostart-Delay"

// 暂时兼容MATE应用
#define GSM_AUTOSTART_APP_ENABLED_KEY "X-MATE-Autostart-enabled"
#define GSM_AUTOSTART_APP_PHASE_KEY "X-MATE-Autostart-Phase"

enum KSMAppEvent
{
    // 应用已退出
    KSM_APP_EVENT_EXITED,
    // 应用启动后已经向会话管理注册成功，表示可以进行下一个阶段的操作
    KSM_APP_EVENT_REGISTERED,
    KSM_APP_EVENT_LAST,
};

class KSMApp : public std::enable_shared_from_this<KSMApp>
{
public:
    KSMApp(const std::string &desktop_file);
    KSMApp(Glib::RefPtr<Gio::DesktopAppInfo> app_info);
    virtual ~KSMApp(){};

    bool start();
    bool restart();
    bool stop();
    bool is_running();
    bool can_launched();

    std::string get_object_path() { return this->object_path_; };
    std::string get_app_id() { return this->app_id_; };
    KSMPhase get_phase() { return this->phase_; };
    std::string get_startup_id() { return this->startup_id_; };
    int32_t get_delay() { return this->delay_; };

    sigc::signal<void, KSMAppEvent> signal_app_event() { return this->app_event_; };

private:
    void load_app_info();

    static void on_launch_cb(GDesktopAppInfo *appinfo, GPid pid, gpointer user_data);
    static void on_app_exited_cb(GPid pid, gint status, gpointer user_data);

private:
    std::string object_path_;
    std::string app_id_;
    KSMPhase phase_;
    Glib::RefPtr<Gio::DesktopAppInfo> app_info_;
    std::string startup_id_;
    // 延时运行时间
    int32_t delay_;

    GPid pid_;
    uint32_t child_watch_id_;
    // TODO: delete
    sigc::signal<void, KSMAppEvent> app_event_;
};

using KSMAppVec = std::vector<std::shared_ptr<KSMApp>>;

}  // namespace Kiran