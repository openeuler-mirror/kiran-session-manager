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

#ifdef __cplusplus
extern "C"
{
#endif

#define DESKTOP_ENVIRONMENT "KIRAN"

/* 为了保持跟第三方应用(firefox、pluma等)兼容，暂时还是使用gnome名字。
   但后续版本有可能更名，所以最好使用宏定义，不要直接使用字符串常量。*/
#define KSM_DBUS_NAME "org.gnome.SessionManager"
#define KSM_DBUS_OBJECT_PATH "/org/gnome/SessionManager"
#define KSM_DBUS_INTERFACE_NAME "org.gnome.SessionManager"

#define KSM_CLIENT_PRIVATE_DBUS_INTERFACE "org.gnome.SessionManager.ClientPrivate"

#define KSM_PRESENCE_DBUS_OBJECT_PATH "/org/gnome/SessionManager/Presence"
#define KSM_PRESENCE_DBUS_INTERFACE_NAME "org.gnome.SessionManager.Presence"

#define KSM_IDLE_DBUS_NAME "com.kylinsec.Kiran.SessionManager.IdleMonitor"
#define KSM_IDLE_DBUS_OBJECT_PATH "/com/kylinsec/Kiran/SessionManager/IdleMonitor"

#define KSM_SCHEMA_ID "com.kylinsec.kiran.session-manager"
// 计算机多久未操作视为空闲
#define KSM_SCHEMA_KEY_IDLE_DELAY "idleDelay"
// 待机时是否锁定屏幕
#define KSM_SCHEMA_KEY_SCREEN_LOCKED_WHEN_SUSPEND "screenLockedWhenSuspend"
// 休眠时是否锁定屏幕
#define KSM_SCHEMA_KEY_SCREEN_LOCKED_WHEN_HIBERNATE "screenLockedWhenHibernate"
// 退出时是否显示退出窗口
#define KSM_SCHEMA_KEY_ALWAYS_SHOW_EXIT_WINDOW "alwaysShowExitWindow"
// 退出窗口倒计时时间
#define KSM_SCHEMA_KEY_EXIT_WINDOW_COUNTDOWN_TIMEOUT "exitWindowCountdownTimeout"

// JK: json key
#define KSM_INHIBITOR_JK_COOKIE "cookie"
#define KSM_INHIBITOR_JK_APP_ID "app_id"
#define KSM_INHIBITOR_JK_TOPLEVEL_XID "toplevel_xid"
#define KSM_INHIBITOR_JK_REASON "reason"
#define KSM_INHIBITOR_JK_FLAGS "flags"

    // 抑制器标记位
    enum KSMInhibitorFlag
    {
        // 抑制注销、关机和重启
        KSM_INHIBITOR_FLAG_QUIT = 1 << 0,
        // 抑制切换用户
        KSM_INHIBITOR_FLAG_SWITCH_USER = 1 << 1,
        // 抑制休眠、挂起和待机
        KSM_INHIBITOR_FLAG_SAVE = 1 << 2,
        // 一直空闲超时信号发送
        KSM_INHIBITOR_FLAG_IDLE = 1 << 3
    };

    // 暂时兼容旧的gnome规范，新的会话管理部分状态未使用
    enum KSMPresenceStatus
    {
        // 正常可用状态
        KSM_PRESENCE_STATUS_AVAILABLE = 0,
        // 未使用
        KSM_PRESENCE_STATUS_INVISIBLE,
        // 未使用
        KSM_PRESENCE_STATUS_BUSY,
        // 空闲状态
        KSM_PRESENCE_STATUS_IDLE,
        KSM_PRESENCE_STATUS_LAST,
    };

    enum PowerAction
    {
        POWER_ACTION_NONE,
        // 切换用户
        POWER_ACTION_SWITCH_USER,
        // 注销
        POWER_ACTION_LOGOUT,
        // 挂起
        POWER_ACTION_SUSPEND,
        // 休眠
        POWER_ACTION_HIBERNATE,
        // 关机
        POWER_ACTION_SHUTDOWN,
        // 重启
        POWER_ACTION_REBOOT,
    };

#ifdef __cplusplus
}
#endif
