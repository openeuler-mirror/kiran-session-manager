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

#include "src/ksm-app.h"
#include <gio/gdesktopappinfo.h>
#include "src/ksm-utils.h"

namespace Kiran
{
KSMApp::KSMApp(const std::string &desktop_file)
{
    auto basename = Glib::path_get_basename(desktop_file);
    auto app_info = Gio::DesktopAppInfo::create(basename);
    new (this) KSMApp(app_info);
}

KSMApp::KSMApp(Glib::RefPtr<Gio::DesktopAppInfo> app_info) : phase_(KSMPhase::KSM_PHASE_APPLICATION),
                                                             app_info_(app_info),
                                                             delay_(-1),
                                                             pid_(-1),
                                                             child_watch_id_(0)
{
    this->load_app_info();
}

bool KSMApp::start()
{
    /* DesktopAppInfo的接口不太完善，如果通过DBUS激活应用则拿不到进程ID，需要自己去DBUS总线获取，但是
       DesktopAppInfo又未暴露应用的dbus name，所以暂时先不支持DBUS启动。*/

    GError *error = NULL;

    if (this->app_info_->get_boolean("DBusActivatable"))
    {
        KLOG_WARNING("DBus startup mode is not supported at present.");
        return false;
    }

    auto launch_context = Gio::AppLaunchContext::create();
    launch_context->setenv("DESKTOP_AUTOSTART_ID", this->startup_id_);

    KLOG_DEBUG("Start app %s, startup id: %s.", this->app_id_.c_str(), this->startup_id_.c_str());
    if (!g_desktop_app_info_launch_uris_as_manager(this->app_info_->gobj(),
                                                   NULL,
                                                   launch_context->gobj(),
                                                   GSpawnFlags(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH),
                                                   NULL,
                                                   NULL,
                                                   &KSMApp::on_launch_cb,
                                                   this,
                                                   &error))
    {
        KLOG_WARNING("Failed to launch app %s: %s.", this->app_id_.c_str(), error->message);
        return false;
    }
    else
    {
        // 这里传递this指针存在一定的风险，需要确保调用回调函数时该对象未被销毁
        this->child_watch_id_ = g_child_watch_add(this->pid_,
                                                  &KSMApp::on_app_exited_cb,
                                                  this);
    }

    return true;
}

bool KSMApp::restart()
{
    this->stop();
    return this->start();
}

bool KSMApp::stop()
{
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

bool KSMApp::can_launched()
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

void KSMApp::load_app_info()
{
    RETURN_IF_FALSE(this->app_info_);

    this->app_id_ = this->app_info_->get_id();

    auto phase_str = this->app_info_->get_string(KSM_AUTOSTART_APP_PHASE_KEY);
    if (phase_str.empty())
    {
        phase_str = this->app_info_->get_string(GSM_AUTOSTART_APP_PHASE_KEY);
    }

    this->phase_ = KSMUtils::phase_str2enum(phase_str);
    this->startup_id_ = KSMUtils::generate_startup_id();

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

void KSMApp::on_launch_cb(GDesktopAppInfo *appinfo, GPid pid, gpointer user_data)
{
    KSMApp *app = (KSMApp *)(user_data);
    app->pid_ = pid;
}

void KSMApp::on_app_exited_cb(GPid pid, gint status, gpointer user_data)
{
    KSMApp *app = (KSMApp *)(user_data);

    Glib::spawn_close_pid(pid);
    app->pid_ = -1;
    app->child_watch_id_ = 0;

    if (WIFEXITED(status))
    {
        KLOG_DEBUG("The app %s exits. exit code: %d.", app->get_app_id().c_str(), WEXITSTATUS(status));
        app->app_event_.emit(KSMAppEvent::KSM_APP_EVENT_EXITED);
    }
    else if (WIFSIGNALED(status))
    {
        KLOG_WARNING("The app %s exits abnormal, exit code: %d.", app->get_app_id().c_str(), WTERMSIG(status));
    }
}

}  // namespace Kiran