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
namespace Daemon
{
#define KSM_AUTOSTART_APP_ENABLED_KEY "X-KIRAN-Autostart-enabled"
#define KSM_AUTOSTART_APP_PHASE_KEY "X-KIRAN-Autostart-Phase"
#define KSM_AUTOSTART_APP_AUTORESTART_KEY "X-KIRAN-AutoRestart"
#define KSM_AUTOSTART_APP_DELAY_KEY "X-KIRAN-Autostart-Delay"

// 暂时兼容MATE应用
#define GSM_AUTOSTART_APP_ENABLED_KEY "X-MATE-Autostart-enabled"
#define GSM_AUTOSTART_APP_AUTORESTART_KEY "X-MATE-AutoRestart"
#define GSM_AUTOSTART_APP_PHASE_KEY "X-MATE-Autostart-Phase"

class App : public std::enable_shared_from_this<App>
{
public:
    App(const std::string &desktop_file);
    App(Glib::RefPtr<Gio::DesktopAppInfo> app_info);
    virtual ~App();

    bool start();
    bool restart();
    bool stop();
    bool is_running();
    bool can_launched();

    std::string get_object_path() { return this->object_path_; };
    std::string get_app_id() { return this->app_id_; };
    KSMPhase get_phase() { return this->phase_; };
    std::string get_startup_id() { return this->startup_id_; };
    bool get_auto_restart() { return this->auto_restart_; };
    int32_t get_delay() { return this->delay_; };

    sigc::signal<void> signal_app_exited() { return this->app_exited_; };

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
    // 正常退出后是否自动重启
    bool auto_restart_;
    // 延时运行时间
    int32_t delay_;

    GPid pid_;
    uint32_t child_watch_id_;
    sigc::signal<void> app_exited_;
};

using KSMAppVec = std::vector<std::shared_ptr<App>>;

}  // namespace Daemon
}  // namespace Kiran