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

#include "src/app/app-manager.h"
#include "src/app/app.h"
#include "src/utils.h"

namespace Kiran
{
namespace Daemon
{
AppManager::AppManager()
{
    this->settings_ = Gio::Settings::create(KSM_SCHEMA_ID);
}

AppManager *AppManager::instance_ = nullptr;
void AppManager::global_init()
{
    instance_ = new AppManager();
    instance_->init();
}

std::shared_ptr<App> AppManager::get_app_by_startup_id(const std::string &startup_id)
{
    KLOG_PROFILE("startup id: %s.", startup_id.c_str());

    for (auto iter : this->apps_)
    {
        if (iter.second->get_startup_id() == startup_id)
        {
            return iter.second;
        }
    }
    return nullptr;
}

KSMAppVec AppManager::start_apps(KSMPhase phase)
{
    KSMAppVec apps;

    for (auto app : this->get_apps_by_phase(phase))
    {
        if (!app->can_launched())
        {
            KLOG_DEBUG("The app %s cannot be launched.", app->get_app_id().c_str());
            continue;
        }

        // 如果应用由设置延时执行，则添加定时器延时启动应用
        auto delay = app->get_delay();
        if (delay > 0)
        {
            auto timeout = Glib::MainContext::get_default()->signal_timeout();
            timeout.connect_seconds([app]() -> bool {
                if (app->can_launched())
                {
                    app->start();
                }
                return false;
            },
                                    delay);
            KLOG_DEBUG("The app is scheduled to start after %d seconds.", app->get_app_id().c_str(), delay);
        }
        else
        {
            CONTINUE_IF_FALSE(app->start());
            KLOG_DEBUG("The app %s is started.", app->get_app_id().c_str());
        }
        apps.push_back(app);
    }
    return apps;
}

void AppManager::init()
{
    this->load_apps();
}

void AppManager::load_apps()
{
    this->load_autostart_apps();
    this->load_required_apps();
}

void AppManager::load_required_apps()
{
    // 会话后端
    // auto session_daemons = this->settings_->get_string_array(KSM_SCHEMA_KEY_SESSION_DAEMONS);
    // for (auto session_daemon : session_daemons)
    // {
    //     auto app_info = Gio::DesktopAppInfo::create(session_daemon + ".desktop");
    //     this->add_app(app_info);
    // }

    // 窗口管理器
    auto window_manager = this->settings_->get_string(KSM_SCHEMA_KEY_WINDOW_MANAGER);
    if (!window_manager.empty())
    {
        auto app_info = Gio::DesktopAppInfo::create(window_manager + ".desktop");
        this->add_app(app_info);
    }
    else
    {
        KLOG_WARNING("The window manager isn't set.");
    }

    // 底部面板
    auto panel = this->settings_->get_string(KSM_SCHEMA_KEY_PANEL);
    if (!panel.empty())
    {
        auto app_info = Gio::DesktopAppInfo::create(panel + ".desktop");
        this->add_app(app_info);
    }
    else
    {
        KLOG_WARNING("The panel isn't set.");
    }

    // 文件管理器
    auto file_manager = this->settings_->get_string(KSM_SCHEMA_KEY_FILE_MANAGER);
    if (!file_manager.empty())
    {
        auto app_info = Gio::DesktopAppInfo::create(file_manager + ".desktop");
        this->add_app(app_info);
    }
    else
    {
        KLOG_WARNING("The file manager isn't set.");
    }
}

void AppManager::load_autostart_apps()
{
    for (auto autostart_dir : Utils::get_autostart_dirs())
    {
        try
        {
            Glib::Dir dir(autostart_dir);
            for (const auto &file_name : dir)
            {
                auto file_path = Glib::build_filename(autostart_dir, file_name);
                auto app_info = Gio::DesktopAppInfo::create_from_filename(file_path);
                this->add_app(app_info);
            }
        }
        catch (const Glib::Error &e)
        {
            KLOG_WARNING("%s.", e.what().c_str());
        }
    }
}

bool AppManager::add_app(Glib::RefPtr<Gio::DesktopAppInfo> app_info)
{
    RETURN_VAL_IF_FALSE(app_info, false);
    RETURN_VAL_IF_TRUE(app_info->gobj() == NULL, false);

    auto app = std::make_shared<App>(app_info);
    auto iter = this->apps_.emplace(app->get_app_id(), app);
    if (!iter.second)
    {
        KLOG_WARNING("The app %s already exist.", app->get_app_id().c_str());
        return false;
    }
    app->signal_app_exited().connect(sigc::bind(sigc::mem_fun(this, &AppManager::on_app_exited_cb), app->get_app_id()));
    return true;
}

KSMAppVec AppManager::get_apps_by_phase(KSMPhase phase)
{
    KSMAppVec apps;
    for (auto iter : this->apps_)
    {
        if (iter.second->get_phase() == phase)
        {
            apps.push_back(iter.second);
        }
    }
    return apps;
}

void AppManager::on_app_exited_cb(std::string app_id)
{
    auto app = this->get_app(app_id);
    RETURN_IF_FALSE(app);
    this->app_exited_.emit(app);
}
}  // namespace Daemon
}  // namespace Kiran
