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
};

using KSMInhibitorVec = std::vector<std::shared_ptr<Inhibitor>>;

class Inhibitors
{
public:
    Inhibitors();
    virtual ~Inhibitors(){};

    // 获取抑制器
    std::shared_ptr<Inhibitor> get_inhibitor(uint32_t cookie) { return MapHelper::get_value(this->inhibitors_, cookie); };
    KSMInhibitorVec get_inhibitors() { return MapHelper::get_values(this->inhibitors_); };
    // 添加抑制器
    std::shared_ptr<Inhibitor> add_inhibitor(const std::string &app_id,
                                             uint32_t toplevel_xid,
                                             const std::string &reason,
                                             uint32_t flags);
    bool add_inhibitor(std::shared_ptr<Inhibitor> inhibitor);
    // 删除抑制器
    void delete_inhibitor(uint32_t cookie);
    // 存在指定类型的抑制器
    bool has_inhibitor(uint32_t flags);

private:
    void update_presence();

private:
    // 抑制器 <cookie, Inhibitor>
    std::map<uint32_t, std::shared_ptr<Inhibitor>> inhibitors_;
};
}  // namespace Daemon
}  // namespace Kiran