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

#include "src/app/app.h"

namespace Kiran
{
namespace Daemon
{
class App;
class AppManager
{
public:
    AppManager();
    virtual ~AppManager(){};

    static AppManager* get_instance() { return instance_; };

    static void global_init();

    static void global_deinit() { delete instance_; };

    // 获取APP
    std::shared_ptr<App> get_app(const std::string& app_id) { return MapHelper::get_value(this->apps_, app_id); };
    std::shared_ptr<App> get_app_by_startup_id(const std::string& startup_id);

    // 启动特定阶段的app，并返回启动成功或者延时启动的app列表
    KSMAppVec start_apps(KSMPhase phase);

    sigc::signal<void, std::shared_ptr<App>> signal_app_exited() { return this->app_exited_; };

private:
    void init();

    void load_apps();
    void load_required_apps();
    void load_autostart_apps();
    // 根据desktopinfo添加app
    bool add_app(Glib::RefPtr<Gio::DesktopAppInfo> app_info);
    // 获取在特定阶段启动的app
    KSMAppVec get_apps_by_phase(KSMPhase phase);

    void on_app_exited_cb(std::string app_id);

private:
    static AppManager* instance_;

    Glib::RefPtr<Gio::Settings> settings_;
    // 开机启动应用
    std::map<std::string, std::shared_ptr<App>> apps_;

    sigc::signal<void, std::shared_ptr<App>> app_exited_;
};
}  // namespace Daemon
}  // namespace Kiran
