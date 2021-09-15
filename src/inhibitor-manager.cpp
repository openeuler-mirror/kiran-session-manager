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

#include "src/inhibitor-manager.h"
#include "src/session-manager.h"
#include "src/utils.h"

namespace Kiran
{
namespace Daemon
{
InhibitorManager::InhibitorManager()
{
}

InhibitorManager *InhibitorManager::instance_ = nullptr;
void InhibitorManager::global_init()
{
    instance_ = new InhibitorManager();
}

std::shared_ptr<Inhibitor> InhibitorManager::add_inhibitor(const std::string &app_id,
                                                           uint32_t toplevel_xid,
                                                           const std::string &reason,
                                                           uint32_t flags,
                                                           const std::string &startup_id)
{
    // 生成一个不冲突的cookie，最多尝试10次。
    const static int32_t COOKIE_MAX_RETRY_COUNT = 10;
    uint32_t cookie = 0;
    int32_t i = 0;
    for (i = 0; i < COOKIE_MAX_RETRY_COUNT; ++i)
    {
        cookie = Utils::generate_cookie();
        auto inhibitor = this->get_inhibitor(cookie);
        if (!inhibitor)
        {
            break;
        }
    }

    RETURN_VAL_IF_TRUE(i == COOKIE_MAX_RETRY_COUNT, nullptr);

    auto inhibitor = std::make_shared<Inhibitor>(Inhibitor{.cookie = cookie,
                                                           .app_id = app_id,
                                                           .toplevel_xid = toplevel_xid,
                                                           .reason = reason,
                                                           .flags = flags,
                                                           .startup_id = startup_id});

    RETURN_VAL_IF_FALSE(this->add_inhibitor(inhibitor), nullptr);
    return inhibitor;
}

void InhibitorManager::delete_inhibitor(uint32_t cookie)
{
    KLOG_DEBUG("cookie: %d.", cookie);

    auto iter = this->inhibitors_.find(cookie);
    if (iter != this->inhibitors_.end())
    {
        auto inhibitor = iter->second;

        this->inhibitors_.erase(iter);
        this->update_presence();
        this->inhibitor_deleted_.emit(inhibitor);
    }
}

void InhibitorManager::delete_inhibitor_by_startup_id(const std::string &startup_id)
{
    std::vector<uint32_t> deleted_cookies;
    for (const auto &iter : this->inhibitors_)
    {
        if (iter.second->startup_id == startup_id)
        {
            deleted_cookies.push_back(iter.second->cookie);
        }
    }

    for (auto deleted_cookie : deleted_cookies)
    {
        this->delete_inhibitor(deleted_cookie);
    }
}

void InhibitorManager::delete_inhibitors_with_startup_id()
{
    std::vector<uint32_t> deleted_cookies;
    for (const auto &iter : this->inhibitors_)
    {
        if (!iter.second->startup_id.empty())
        {
            deleted_cookies.push_back(iter.second->cookie);
        }
    }

    for (auto deleted_cookie : deleted_cookies)
    {
        this->delete_inhibitor(deleted_cookie);
    }
}

bool InhibitorManager::has_inhibitor(uint32_t flags)
{
    auto iter = std::find_if(this->inhibitors_.begin(), this->inhibitors_.end(), [flags](std::pair<uint32_t, std::shared_ptr<Inhibitor>> iter) {
        return ((flags & iter.second->flags) == flags);
    });

    return (iter != this->inhibitors_.end());
}

bool InhibitorManager::add_inhibitor(std::shared_ptr<Inhibitor> inhibitor)
{
    RETURN_VAL_IF_FALSE(inhibitor, false);

    KLOG_DEBUG("cookie: %d.", inhibitor->cookie);

    auto iter = this->inhibitors_.emplace(inhibitor->cookie, inhibitor);
    if (!iter.second)
    {
        KLOG_WARNING("The inhibitor %d already exist.", inhibitor->cookie);
        return false;
    }
    this->update_presence();

    this->inhibitor_added_.emit(inhibitor);
    return true;
}

void InhibitorManager::update_presence()
{
    auto presence = SessionManager::get_instance()->get_presence();
    if (this->has_inhibitor(KSMInhibitorFlag::KSM_INHIBITOR_FLAG_IDLE))
    {
        presence->enable_idle_timeout(false);
    }
    else
    {
        presence->enable_idle_timeout(true);
    }
}
}  // namespace Daemon
}  // namespace Kiran
