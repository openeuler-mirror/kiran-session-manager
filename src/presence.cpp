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

#include "src/presence.h"

namespace Kiran
{
namespace Daemon
{
#define KSM_SCHEMA_ID "com.kylinsec.kiran.session-manager"
#define KSM_SCHEMA_KEY_IDLE_DELAY "idle-delay"

Presence::Presence() : enabled_idle_timeout_(true),
                       idle_timeout_(0),
                       status_(KSMPresenceStatus::KSM_PRESENCE_STATUS_AVAILABLE),
                       dbus_connect_id_(0),
                       object_register_id_(0)
{
    this->settings_ = Gio::Settings::create(KSM_SCHEMA_ID);
}

void Presence::init()
{
    this->idle_timeout_ = this->settings_->get_int(KSM_SCHEMA_KEY_IDLE_DELAY);
    this->idle_xlarm_.init();
    this->update_idle_xlarm();

    this->idle_xlarm_.signal_alarm_triggered().connect(sigc::mem_fun(this, &Presence::on_alarm_triggered_cb));
    this->idle_xlarm_.signal_alarm_reset().connect(sigc::mem_fun(this, &Presence::on_alarm_reset_cb));
    this->settings_->signal_changed().connect(sigc::mem_fun(this, &Presence::on_settings_changed));

    this->dbus_connect_id_ = Gio::DBus::own_name(Gio::DBus::BUS_TYPE_SESSION,
                                                 KSM_DBUS_NAME,
                                                 sigc::mem_fun(this, &Presence::on_bus_acquired),
                                                 sigc::mem_fun(this, &Presence::on_name_acquired),
                                                 sigc::mem_fun(this, &Presence::on_name_lost));
}

void Presence::enable_idle_timeout(bool enabled)
{
    KLOG_PROFILE("enabled: %d.", enabled);

    RETURN_IF_TRUE(this->enabled_idle_timeout_ == enabled);
    this->enabled_idle_timeout_ = enabled;
    this->update_idle_xlarm();
}

void Presence::SetStatus(guint32 status, MethodInvocation &invocation)
{
    KLOG_PROFILE("status: %d.", status);

    if (status >= KSM_PRESENCE_STATUS_LAST)
    {
        DBUS_ERROR_REPLY_AND_RET(KSMErrorCode::ERROR_PRESENCE_STATUS_INVALID);
    }
    // 这里忽略status相同导致的设置失败的情况
    this->status_set(status);
    invocation.ret();
}

void Presence::SetStatusText(const Glib::ustring &status_text, MethodInvocation &invocation)
{
    KLOG_PROFILE("status text: %s.", status_text.c_str());
    this->status_text_set(status_text);
    invocation.ret();
}

bool Presence::status_setHandler(guint32 value)
{
    RETURN_VAL_IF_FALSE(this->status_ != value, false);
    this->status_ = value;
    this->StatusChanged_signal.emit(this->status_);
    return true;
}

bool Presence::status_text_setHandler(const Glib::ustring &value)
{
    RETURN_VAL_IF_FALSE(this->status_text_ != value, false);
    this->status_text_ = value;
    this->StatusTextChanged_signal.emit(this->status_text_);
    return true;
}

void Presence::update_idle_xlarm()
{
    KLOG_PROFILE("enabled: %d, idle timeout: %d.",
                 this->enabled_idle_timeout_,
                 this->idle_timeout_);

    if (!this->enabled_idle_timeout_ || this->idle_timeout_ <= 0)
    {
        this->idle_xlarm_.unset(XAlarmType::XALARM_TYPE_IDLE_TIMEOUT);
    }
    else
    {
        this->idle_xlarm_.set(XAlarmType::XALARM_TYPE_IDLE_TIMEOUT, this->idle_timeout_ * 60000);
    }
}

void Presence::on_alarm_triggered_cb(std::shared_ptr<XAlarmInfo> xalarm)
{
    KLOG_PROFILE("");

    RETURN_IF_FALSE(xalarm);
    if (xalarm->type == XALARM_TYPE_IDLE_TIMEOUT)
    {
        this->status_set(KSMPresenceStatus::KSM_PRESENCE_STATUS_IDLE);
    }
}

void Presence::on_alarm_reset_cb()
{
    KLOG_PROFILE("");
    this->status_set(KSMPresenceStatus::KSM_PRESENCE_STATUS_AVAILABLE);
}

void Presence::on_settings_changed(const Glib::ustring &key)
{
    KLOG_PROFILE("");

    switch (shash(key.c_str()))
    {
    case CONNECT(KSM_SCHEMA_KEY_IDLE_DELAY, _hash):
    {
        this->idle_timeout_ = this->settings_->get_int(key);
        this->update_idle_xlarm();
        break;
    }
    default:
        break;
    }
}

void Presence::on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name)
{
    if (!connect)
    {
        KLOG_WARNING("Failed to connect dbus. name: %s", name.c_str());
        return;
    }
    try
    {
        this->object_register_id_ = this->register_object(connect, KSM_PRESENCE_DBUS_OBJECT_PATH);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("Register object_path %s fail: %s.", KSM_PRESENCE_DBUS_OBJECT_PATH, e.what().c_str());
    }
}

void Presence::on_name_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name)
{
    KLOG_DEBUG("Success to register dbus name: %s", name.c_str());
}

void Presence::on_name_lost(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name)
{
    KLOG_WARNING("Failed to register dbus name: %s", name.c_str());
}

}  // namespace Daemon
}  // namespace Kiran
