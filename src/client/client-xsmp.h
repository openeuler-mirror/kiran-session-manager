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

#include <X11/SM/SMlib.h>
#include "src/client/client.h"

namespace Kiran
{
namespace Daemon
{
// struct XsmpClientProps
// {
//     XsmpClientProps() : process_id(-1), restart_style_hint(-1) {}

//     std::string discard_command;
//     int32_t process_id;
//     // 程序名
//     std::string program;
//     std::string restart_command;
//     int32_t restart_style_hint;
// };

class ClientXsmp : public Client
{
public:
    ClientXsmp(const std::string &startup_id, SmsConn sms_connection);
    virtual ~ClientXsmp();

    ClientType get_type() { return ClientType::CLIENT_TYPE_XSMP; };
    SmsConn get_sms_connection() { return this->sms_connection_; };

    virtual bool cancel_end_session();
    virtual bool query_end_session(bool interact);
    virtual bool end_session(bool save_data);
    virtual bool end_session_phase2();
    virtual bool stop();

    // 添加/更新/删除/查找属性
    void update_property(SmProp *property);
    void delete_property(const std::string &property_name);
    SmProp *get_property(const std::string &property_name);
    GPtrArray *get_properties() { return this->props_; };

    // 获取程序名
    std::string get_program_name();

    sigc::signal<void> signal_register_request() { return this->register_request_; };
    sigc::signal<void> signal_logout_request() { return this->logout_request_; };

private:
    int32_t get_property_index(const std::string &property_name);
    std::string prop_to_command(SmProp *prop);
    // void do_save_yourself(int32_t save_type, bool interact);

private:
    SmsConn sms_connection_;

    std::string app_id_;
    uint32_t status_;

    // XsmpClientProps props_;
    // XSMP的SmsGetPropertiesProcMask回调处理需要返回SmProp指针数组，因此这里用GPtrArray比较方便
    GPtrArray *props_;

    int32_t current_save_yourself_;
    int32_t next_save_yourself_;
    bool next_save_yourself_allow_interact_;

    sigc::signal<void> register_request_;
    sigc::signal<void> logout_request_;
};
}  // namespace Daemon
}  // namespace Kiran
