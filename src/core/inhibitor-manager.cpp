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
    m_presence = new Presence(this);
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
    for (auto &iter : m_inhibitors)
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
        auto inhibitor = getInhibitor(cookie);
        if (!inhibitor)
        {
            break;
        }
    }

    RETURN_VAL_IF_TRUE(i == COOKIE_MAX_RETRY_COUNT, nullptr);

    auto inhibitor = QSharedPointer<Inhibitor>(new Inhibitor(cookie, appID, toplevelXID, reason, flags, startupID));

    RETURN_VAL_IF_FALSE(addInhibitor(inhibitor), nullptr);
    return inhibitor;
}

void InhibitorManager::deleteInhibitor(uint32_t cookie)
{
    auto iter = m_inhibitors.find(cookie);
    if (iter != m_inhibitors.end())
    {
        auto inhibitor = iter.value();

        m_inhibitors.erase(iter);
        updatePresence();
        Q_EMIT inhibitorDeleted(inhibitor);
    }
}

void InhibitorManager::deleteInhibitorByStartupID(const QString &startupID)
{
    std::vector<uint32_t> deletedCookies;
    for (const auto &iter : m_inhibitors)
    {
        if (iter->startupID == startupID)
        {
            deletedCookies.push_back(iter->cookie);
        }
    }

    for (auto deletedCookie : deletedCookies)
    {
        deleteInhibitor(deletedCookie);
    }
}

void InhibitorManager::deleteInhibitorsWithStartupID()
{
    std::vector<uint32_t> deletedCookies;
    for (const auto &iter : m_inhibitors)
    {
        if (!iter->startupID.isEmpty())
        {
            deletedCookies.push_back(iter->cookie);
        }
    }

    for (auto deleted_cookie : deletedCookies)
    {
        deleteInhibitor(deleted_cookie);
    }
}

bool InhibitorManager::hasInhibitor(uint32_t flags)
{
    auto iter = std::find_if(m_inhibitors.begin(), m_inhibitors.end(), [flags](QSharedPointer<Inhibitor> iter)
                             { return ((flags & iter->flags) == flags); });

    return (iter != m_inhibitors.end());
}

void InhibitorManager::init()
{
    m_presence->init();
}

bool InhibitorManager::addInhibitor(QSharedPointer<Inhibitor> inhibitor)
{
    RETURN_VAL_IF_FALSE(inhibitor, false);

    KLOG_INFO() << "Add a new inhibitor:"
                << QString("cookie=%1, appid=%2, flags=%3, reason=%4, startupID=%5, toplevelXID=%6")
                       .arg(inhibitor->cookie)
                       .arg(inhibitor->appID)
                       .arg(inhibitor->flags)
                       .arg(inhibitor->reason)
                       .arg(inhibitor->startupID)
                       .arg(inhibitor->toplevelXID);

    if (m_inhibitors.find(inhibitor->cookie) != m_inhibitors.end())
    {
        KLOG_WARNING() << "The inhibitor" << inhibitor->cookie << "already exist.";
        return false;
    }

    m_inhibitors.insert(inhibitor->cookie, inhibitor);
    updatePresence();

    Q_EMIT inhibitorAdded(inhibitor);
    return true;
}

void InhibitorManager::updatePresence()
{
    if (hasInhibitor(KSMInhibitorFlag::KSM_INHIBITOR_FLAG_IDLE))
    {
        m_presence->enableIdleTimeout(false);
    }
    else
    {
        m_presence->enableIdleTimeout(true);
    }
}
}  // namespace Kiran
