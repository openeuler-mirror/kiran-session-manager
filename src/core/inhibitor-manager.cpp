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

#include "inhibitor-manager.h"
#include "lib/base/base.h"
#include "presence.h"
#include "session-manager.h"
#include "utils.h"

namespace Kiran
{
InhibitorManager::InhibitorManager() : QObject(nullptr)
{
    this->m_presence = new Presence(this);
}

InhibitorManager *InhibitorManager::m_instance = nullptr;
void InhibitorManager::globalInit()
{
    m_instance = new InhibitorManager();
    m_instance->init();
}

KSMInhibitorVec InhibitorManager::getInhibitorsByFlag(KSMInhibitorFlag flag)
{
    KSMInhibitorVec inhibitors;
    for (auto &iter : this->m_inhibitors)
    {
        if ((iter->flags & flag) == flag)
        {
            inhibitors.push_back(iter);
        }
    }
    return inhibitors;
}

QSharedPointer<Inhibitor> InhibitorManager::addInhibitor(const QString &appID,
                                                         uint32_t toplevelXID,
                                                         const QString &reason,
                                                         uint32_t flags,
                                                         const QString &startupID)
{
    // 生成一个不冲突的cookie，最多尝试10次。
    const static int32_t COOKIE_MAX_RETRY_COUNT = 10;
    uint32_t cookie = 0;
    int32_t i = 0;
    for (i = 0; i < COOKIE_MAX_RETRY_COUNT; ++i)
    {
        cookie = Utils::getDefault()->generateCookie();
        auto inhibitor = this->getInhibitor(cookie);
        if (!inhibitor)
        {
            break;
        }
    }

    RETURN_VAL_IF_TRUE(i == COOKIE_MAX_RETRY_COUNT, nullptr);

    auto inhibitor = QSharedPointer<Inhibitor>(new Inhibitor{.cookie = cookie,
                                                             .appID = appID,
                                                             .toplevelXID = toplevelXID,
                                                             .reason = reason,
                                                             .flags = flags,
                                                             .startupID = startupID});

    RETURN_VAL_IF_FALSE(this->addInhibitor(inhibitor), nullptr);
    return inhibitor;
}

void InhibitorManager::deleteInhibitor(uint32_t cookie)
{
    KLOG_DEBUG("cookie: %d.", cookie);

    auto iter = this->m_inhibitors.find(cookie);
    if (iter != this->m_inhibitors.end())
    {
        auto inhibitor = iter.value();

        this->m_inhibitors.erase(iter);
        this->updatePresence();
        Q_EMIT this->inhibitorDeleted(inhibitor);
    }
}

void InhibitorManager::deleteInhibitorByStartupID(const QString &startupID)
{
    std::vector<uint32_t> deletedCookies;
    for (const auto &iter : this->m_inhibitors)
    {
        if (iter->startupID == startupID)
        {
            deletedCookies.push_back(iter->cookie);
        }
    }

    for (auto deletedCookie : deletedCookies)
    {
        this->deleteInhibitor(deletedCookie);
    }
}

void InhibitorManager::deleteInhibitorsWithStartupID()
{
    std::vector<uint32_t> deletedCookies;
    for (const auto &iter : this->m_inhibitors)
    {
        if (!iter->startupID.isEmpty())
        {
            deletedCookies.push_back(iter->cookie);
        }
    }

    for (auto deleted_cookie : deletedCookies)
    {
        this->deleteInhibitor(deleted_cookie);
    }
}

bool InhibitorManager::hasInhibitor(uint32_t flags)
{
    auto iter = std::find_if(this->m_inhibitors.begin(), this->m_inhibitors.end(), [flags](QSharedPointer<Inhibitor> iter)
                             { return ((flags & iter->flags) == flags); });

    return (iter != this->m_inhibitors.end());
}

void InhibitorManager::init()
{
    this->m_presence->init();
}

bool InhibitorManager::addInhibitor(QSharedPointer<Inhibitor> inhibitor)
{
    RETURN_VAL_IF_FALSE(inhibitor, false);

    KLOG_DEBUG() << "Cookie: " << inhibitor->cookie;

    KLOG_DEBUG() << "Add a new inhibitor,"
                 << " cookie: " << inhibitor->cookie
                 << ", appid: " << inhibitor->appID
                 << ", flags: " << inhibitor->flags
                 << ", reason: " << inhibitor->reason
                 << ", startupID: " << inhibitor->startupID
                 << ", toplevelXID: " << inhibitor->toplevelXID;

    if (this->m_inhibitors.find(inhibitor->cookie) != this->m_inhibitors.end())
    {
        KLOG_WARNING() << "The inhibitor " << inhibitor->cookie << " already exist.";
        return false;
    }

    this->m_inhibitors.insert(inhibitor->cookie, inhibitor);
    this->updatePresence();

    Q_EMIT this->inhibitorAdded(inhibitor);
    return true;
}

void InhibitorManager::updatePresence()
{
    if (this->hasInhibitor(KSMInhibitorFlag::KSM_INHIBITOR_FLAG_IDLE))
    {
        this->m_presence->enableIdleTimeout(false);
    }
    else
    {
        this->m_presence->enableIdleTimeout(true);
    }
}
}  // namespace Kiran
