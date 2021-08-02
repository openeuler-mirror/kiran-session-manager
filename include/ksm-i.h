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

#define KSM_DBUS_NAME "org.gnome.SessionManager"
#define KSM_DBUS_OBJECT_PATH "/org/gnome/SessionManager"
#define KSM_DBUS_INTERFACE_NAME "org.gnome.SessionManager"

#define KSM_PRESENCE_DBUS_OBJECT_PATH "/org/gnome/SessionManager/Presence"
#define KSM_PRESENCE_DBUS_INTERFACE_NAME "org.gnome.SessionManager.Presence"

#define KSM_SCHEMA_ID "com.kylinsec.kiran.session-manager"
#define KSM_SCHEMA_KEY_SESSION_DAEMONS "session-daemons"
#define KSM_SCHEMA_KEY_WINDOW_MANAGER "window-manager"
#define KSM_SCHEMA_KEY_PANEL "panel"
#define KSM_SCHEMA_KEY_FILE_MANAGER "file-manager"

    enum KSMPhase
    {
        // 未开始
        KSM_PHASE_IDLE = 0,
        // 启动控制中心会话后端
        KSM_PHASE_INITIALIZATION,
        // 启动窗口管理器
        KSM_PHASE_WINDOW_MANAGER,
        // 启动底部面板
        KSM_PHASE_PANEL,
        // 启动文件管理器
        KSM_PHASE_DESKTOP,
        // 启动其他应用程序
        KSM_PHASE_APPLICATION,
        // 会话运行阶段
        KSM_PHASE_RUNNING,
        // 会话结束阶段
        KSM_PHASE_QUERY_END_SESSION,
        // 会话结束第一阶段
        KSM_PHASE_END_SESSION_PHASE1,
        // 会话结束第二阶段
        KSM_PHASE_END_SESSION_PHASE2,
        // 会话结束第二阶段
        KSM_PHASE_EXIT
    };

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
        KSM_PRESENCE_STATUS_AVAILABLE = 0,
        KSM_PRESENCE_STATUS_INVISIBLE,
        KSM_PRESENCE_STATUS_BUSY,
        KSM_PRESENCE_STATUS_IDLE,
        KSM_PRESENCE_STATUS_LAST,
    };

#ifdef __cplusplus
}
#endif