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

#include <gdkmm.h>
//
#include <X11/Xlib.h>
#include <X11/extensions/sync.h>

#include "lib/base.h"

namespace Kiran
{
namespace Daemon
{
// 重置报警器是当显示设备从空闲状态->非空闲状态时被触发。其他报警器是当显示设备的空闲时间超过设定的时间后触发。
enum XAlarmType
{
    // 重置报警器。如果系统从空闲->非空闲，则会触发重置报警器，重置报警器会重新注册或更新其他报警器的信息
    XALARM_TYPE_RESET = 0,
    // 会话管理空闲超时报警器。如果系统从非空闲->空闲，并超过设定的时间，则触发空闲超时报警器。
    XALARM_TYPE_IDLE_TIMEOUT,
    XALARM_TYPE_LAST
};

struct XAlarmInfo
{
    XAlarmInfo(XAlarmType t);

    XAlarmType type;
    XSyncValue timeout;
    XSyncAlarm xalarm_id;
};

using XAlarmInfoVec = std::vector<std::shared_ptr<XAlarmInfo>>;

// 通过XSync协议设置空闲报警器
class IdleXAlarm
{
public:
    IdleXAlarm();
    virtual ~IdleXAlarm();

    void init();

    // 设置一个报警器，如果不存在则添加一个新的报警器，否则更新原有报警器的超时时间，当显示设备超过timeout时间未操作时会收到X发送的Alarm信号
    bool set(XAlarmType type, uint32_t timeout);

    // 删除之前设置的报警器，首先会销毁在X中记录的报警器，然后销毁报警器的XAlarmInfo对象
    bool unset(XAlarmType type);

    // 获取已经空闲的时间
    int64_t get_xidle_time();

    // 报警器触发
    sigc::signal<void, std::shared_ptr<XAlarmInfo>> &signal_alarm_triggered() { return this->alarm_triggered_; };
    // 报警器重置
    sigc::signal<void> &signal_alarm_reset() { return this->alarm_reset_; };

private:
    // 添加一个报警器，如果已经存在则返回失败
    bool add(std::shared_ptr<XAlarmInfo> xalarm);

    // 删除一个报警器
    bool remove(std::shared_ptr<XAlarmInfo> xalarm);

    // 重置所有报警器
    void reset_all_xalarm();

    // 使用XSync协议向X注册报警器
    void register_xalarm_by_xsync(std::shared_ptr<XAlarmInfo> xalarm, XSyncTestType test_type);
    // 使用XSync协议向X销毁报警器
    void unregister_xalarm_by_xsync(std::shared_ptr<XAlarmInfo> xalarm);

    // 查找报警器
    std::shared_ptr<XAlarmInfo> find_xalarm_by_type(XAlarmType type);
    std::shared_ptr<XAlarmInfo> find_xalarm_by_id(XSyncAlarm xalarm_id);

    // XSyncValue转int64_t
    int64_t xsyncvalue_to_int64(XSyncValue value);

    static GdkFilterReturn on_event_filter_cb(GdkXEvent *gdkxevent, GdkEvent *event, gpointer data);

private:
    Display *xdisplay_;

    int32_t sync_event_base_;
    int32_t sync_error_base_;

    XSyncCounter idle_counter_;
    bool added_event_filter_;

    XAlarmInfoVec xalarms_;

    sigc::signal<void, std::shared_ptr<XAlarmInfo>> alarm_triggered_;
    sigc::signal<void> alarm_reset_;
};
}  // namespace Daemon
}  // namespace Kiran