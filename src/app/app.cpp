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

#include "src/app/app.h"
#include <gdkmm.h>
#include <gio/gdesktopappinfo.h>
#include "src/app/app-manager.h"
#include "src/utils.h"

namespace Kiran
{
namespace Daemon
{
#define DESKTOP_KEY_DBUS_ACTIVATABLE "DBusActivatable"
#define DESKTOP_KEY_STARTUP_NOTIFY "StartupNotify"

App::App(const std::string &desktop_file)
{
    auto basename = Glib::path_get_basename(desktop_file);
    auto app_info = Gio::DesktopAppInfo::create(basename);
    new (this) App(app_info);
}

App::~App()
{
    KLOG_DEBUG("App %s is destroyed.", this->app_id_.c_str());

    if (this->child_watch_id_)
    {
        g_source_remove(this->child_watch_id_);
        this->child_watch_id_ = 0;
    }
}

App::App(Glib::RefPtr<Gio::DesktopAppInfo> app_info) : phase_(KSMPhase::KSM_PHASE_APPLICATION),
                                                       app_info_(app_info),
                                                       auto_restart_(false),
                                                       delay_(-1),
                                                       pid_(-1),
                                                       child_watch_id_(0)
{
    this->load_app_info();
}

bool App::start()
{
    /* DesktopAppInfo的接口不太完善，如果通过DBUS激活应用则拿不到进程ID，需要自己去DBUS总线获取，但是
       DesktopAppInfo又未暴露应用的dbus name，所以暂时先不支持DBUS启动。*/

    GError *error = NULL;

    if (this->app_info_->get_boolean(DESKTOP_KEY_DBUS_ACTIVATABLE))
    {
        KLOG_WARNING("DBus startup mode is not supported at present.");
        return false;
    }

    auto launch_context = Gdk::Display::get_default()->get_app_launch_context();
    launch_context->setenv("DESKTOP_AUTOSTART_ID", this->startup_id_);

    KLOG_DEBUG("Start app %s, startup id: %s.", this->app_id_.c_str(), this->startup_id_.c_str());
    if (!g_desktop_app_info_launch_uris_as_manager(this->app_info_->gobj(),
                                                   NULL,
                                                   Glib::RefPtr<Gio::AppLaunchContext>::cast_dynamic(launch_context)->gobj(),
                                                   GSpawnFlags(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH),
                                                   NULL,
                                                   NULL,
                                                   &App::on_launch_cb,
                                                   this,
                                                   &error))
    {
        KLOG_WARNING("Failed to launch app %s: %s.", this->app_id_.c_str(), error->message);
        return false;
    }

    KLOG_DEBUG("Launch child pid: %d.", this->pid_);
    launch_context->unsetenv("DESKTOP_AUTOSTART_ID");

    char *app_id = g_strdup(this->app_id_.c_str());
    this->child_watch_id_ = g_child_watch_add_full(G_PRIORITY_DEFAULT,
                                                   this->pid_,
                                                   &App::on_app_exited_cb,
                                                   app_id,
                                                   g_free);
    return true;
}

bool App::restart()
{
    KLOG_PROFILE("");

    this->stop();
    return this->start();
}

bool App::stop()
{
    KLOG_PROFILE("");

    if (this->pid_ <= 1)
    {
        KLOG_WARNING("The app %s is not running.", this->app_id_.c_str());
        return false;
    }

    auto status = kill(this->pid_, SIGTERM);
    if (status < 0)
    {
        if (errno == ESRCH)
        {
            KLOG_WARNING("Child process %d was already dead.",
                         (int)this->pid_);
        }
        else
        {
            KLOG_WARNING("Couldn't kill child process %d: %s",
                         (int)this->pid_,
                         g_strerror(errno));
        }
        return false;
    }
    return true;
}

bool App::can_launched()
{
    if (this->app_info_->has_key(KSM_AUTOSTART_APP_ENABLED_KEY) &&
        !this->app_info_->get_boolean(KSM_AUTOSTART_APP_ENABLED_KEY))
    {
        KLOG_DEBUG("The app %s is disabled.", this->app_id_.c_str());
        return false;
    }

    if (this->app_info_->has_key(GSM_AUTOSTART_APP_ENABLED_KEY) &&
        !this->app_info_->get_boolean(GSM_AUTOSTART_APP_ENABLED_KEY))
    {
        KLOG_DEBUG("The app %s is disabled.", this->app_id_.c_str());
        return false;
    }

    RETURN_VAL_IF_FALSE(!this->app_info_->is_hidden(), false);

    if (!this->app_info_->get_show_in(DESKTOP_ENVIRONMENT) && !this->app_info_->get_show_in("MATE"))
    {
        KLOG_DEBUG("The app %s doesn't display in the desktop environment.", this->app_id_.c_str());
        return false;
    }

    return true;
}

void App::load_app_info()
{
    RETURN_IF_FALSE(this->app_info_);

    this->startup_id_ = Utils::generate_startup_id();

    // Desktop ID
    this->app_id_ = this->app_info_->get_id();

    // Phase
    auto phase_str = this->app_info_->get_string(KSM_AUTOSTART_APP_PHASE_KEY);
    if (phase_str.empty())
    {
        phase_str = this->app_info_->get_string(GSM_AUTOSTART_APP_PHASE_KEY);
    }
    this->phase_ = Utils::phase_str2enum(phase_str);

    // Auto restart
    if (this->app_info_->has_key(KSM_AUTOSTART_APP_AUTORESTART_KEY))
    {
        this->auto_restart_ = this->app_info_->get_boolean(KSM_AUTOSTART_APP_AUTORESTART_KEY);
    }
    else
    {
        this->auto_restart_ = this->app_info_->get_boolean(GSM_AUTOSTART_APP_AUTORESTART_KEY);
    }

    // Delay
    auto delay_str = this->app_info_->get_string(KSM_AUTOSTART_APP_DELAY_KEY);
    if (!delay_str.empty())
    {
        /* 由于在APPLICATION阶段之前会话管理会设置一个超时定时器在应用程序启动时等待一段时间，
        如果此时设置的延时启动时间大于这个定时器等待的时间，可能会发生一些不可预知的后果，
        因此在APPLICATION阶段之前启动的应用程序暂时不运行设置延时启动。如果一定要设置，那
        这个时间应该要小于会话管理设置的超时定时器的时间。*/
        if (this->phase_ == KSMPhase::KSM_PHASE_APPLICATION)
        {
            this->delay_ = std::strtol(delay_str.c_str(), NULL, 0);
        }
        else
        {
            KLOG_WARNING("Only accept an delay in KSM_PHASE_APPLICATION phase.");
        }
    }
}

void App::on_launch_cb(GDesktopAppInfo *appinfo, GPid pid, gpointer user_data)
{
    App *app = (App *)(user_data);
    app->pid_ = pid;
}

void App::on_app_exited_cb(GPid pid, gint status, gpointer user_data)
{
    auto app_id = (char *)(user_data);
    auto app = AppManager::get_instance()->get_app(POINTER_TO_STRING(app_id));
    auto pid_back = app->pid_;

    Glib::spawn_close_pid(pid);

    if (WIFEXITED(status))
    {
        KLOG_DEBUG("The app %s exits. pid: %d, exit code: %d.", app->get_app_id().c_str(), (int32_t)pid, WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        KLOG_WARNING("The app %s exits abnormal, pid: %d, exit code: %d.", app->get_app_id().c_str(), (int32_t)pid, WTERMSIG(status));
    }

    /* 在程序重启时可能存在一个延时，导致最终函数执行顺序是：restart->stop->start->on_app_exited_cb
        而在调用start函数时已经重新设置pid和child_watch_id了，因此如果pid != app->pid_则不再进行清理操作 */
    if (pid == app->pid_)
    {
        app->pid_ = -1;
        app->child_watch_id_ = 0;
    }

    app->app_exited_.emit();
}
}  // namespace Daemon
}  // namespace Kiran
