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

#include "lib/base.h"

namespace Kiran
{
namespace Daemon
{
struct Inhibitor
{
    uint32_t cookie;
    std::string app_id;
    uint32_t toplevel_xid;
    std::string reason;
    uint32_t flags;
    std::string startup_id;
};

using KSMInhibitorVec = std::vector<std::shared_ptr<Inhibitor>>;

class InhibitorManager
{
public:
    InhibitorManager();
    virtual ~InhibitorManager(){};

    static InhibitorManager *get_instance() { return instance_; };

    static void global_init();

    static void global_deinit() { delete instance_; };

    // 获取抑制器
    std::shared_ptr<Inhibitor> get_inhibitor(uint32_t cookie) { return MapHelper::get_value(this->inhibitors_, cookie); };
    KSMInhibitorVec get_inhibitors() { return MapHelper::get_values(this->inhibitors_); };
    KSMInhibitorVec get_inhibitors_by_flag(KSMInhibitorFlag flag);
    // 添加抑制器
    std::shared_ptr<Inhibitor> add_inhibitor(const std::string &app_id,
                                             uint32_t toplevel_xid,
                                             const std::string &reason,
                                             uint32_t flags,
                                             const std::string &startup_id = std::string());

    // 删除抑制器
    void delete_inhibitor(uint32_t cookie);
    void delete_inhibitor_by_startup_id(const std::string &startup_id);
    void delete_inhibitors_with_startup_id();
    // 存在指定类型的抑制器
    bool has_inhibitor(uint32_t flags);

    sigc::signal<void, std::shared_ptr<Inhibitor>> signal_inhibitor_added() { return this->inhibitor_added_; };
    sigc::signal<void, std::shared_ptr<Inhibitor>> signal_inhibitor_deleted() { return this->inhibitor_deleted_; };

private:
    bool add_inhibitor(std::shared_ptr<Inhibitor> inhibitor);
    void update_presence();

private:
    static InhibitorManager *instance_;

    // 抑制器 <cookie, Inhibitor>
    std::map<uint32_t, std::shared_ptr<Inhibitor>> inhibitors_;

    sigc::signal<void, std::shared_ptr<Inhibitor>> inhibitor_added_;
    sigc::signal<void, std::shared_ptr<Inhibitor>> inhibitor_deleted_;
};
}  // namespace Daemon
}  // namespace Kiran