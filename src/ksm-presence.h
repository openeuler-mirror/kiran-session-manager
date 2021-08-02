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

#include "src/ksm-idle-xalarm.h"

#include <presence_dbus_stub.h>

namespace Kiran
{
class KSMPresence : public gnome::SessionManager::PresenceStub
{
public:
    KSMPresence();
    virtual ~KSMPresence(){};

    void init();

    // 空闲超时是否开启，如果添加了抑制器则不应该触发空闲超时信号
    void enable_idle_timeout(bool enabled);

protected:
    virtual void SetStatus(guint32 status, MethodInvocation &invocation);
    virtual void SetStatusText(const Glib::ustring &status_text, MethodInvocation &invocation);

    virtual bool status_setHandler(guint32 value);
    virtual bool status_text_setHandler(const Glib::ustring &value);

    virtual Glib::ustring status_text_get() { return this->status_text_; };
    virtual guint32 status_get() { return this->status_; };

private:
    void update_idle_xlarm();

    void on_alarm_triggered_cb(std::shared_ptr<XAlarmInfo> xalarm);
    void on_alarm_reset_cb();
    void on_settings_changed(const Glib::ustring &key);

    void on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name);
    void on_name_acquired(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name);
    void on_name_lost(const Glib::RefPtr<Gio::DBus::Connection> &connect, Glib::ustring name);

private:
    KSMIdleXAlarm idle_xlarm_;
    // 空闲超时被开启
    bool enabled_idle_timeout_;
    // 空闲超时时间（毫秒）
    int32_t idle_timeout_;

    uint32_t status_;
    std::string status_text_;

    Glib::RefPtr<Gio::Settings> settings_;

    uint32_t dbus_connect_id_;
    uint32_t object_register_id_;
};
}  // namespace Kiran
