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

#include <ksm-i.h>
#include <QMap>
#include <QObject>
#include <QSharedPointer>

namespace Kiran
{
struct Inhibitor
{
public:
    Inhibitor() : cookie(0), toplevelXID(0), flags(0){};
    Inhibitor(uint32_t cookie,
              QString appID,
              uint32_t toplevelXID,
              QString reason,
              uint32_t flags,
              QString startupID)
    {
        this->cookie = cookie;
        this->appID = appID;
        this->toplevelXID = toplevelXID;
        this->reason = reason;
        this->flags = flags;
        this->startupID = startupID;
    };

    uint32_t cookie;
    QString appID;
    uint32_t toplevelXID;
    QString reason;
    uint32_t flags;
    QString startupID;
};

using KSMInhibitorVec = QList<QSharedPointer<Inhibitor>>;

class Presence;

class InhibitorManager : public QObject
{
    Q_OBJECT
public:
    InhibitorManager();
    virtual ~InhibitorManager(){};

    static InhibitorManager *getInstance() { return m_instance; };

    static void globalInit();

    static void globalDeinit() { delete m_instance; };

    // 获取抑制器
    QSharedPointer<Inhibitor> getInhibitor(uint32_t cookie) { return this->m_inhibitors.value(cookie, QSharedPointer<Inhibitor>()); };
    KSMInhibitorVec getInhibitors() { return this->m_inhibitors.values(); };
    KSMInhibitorVec getInhibitorsByFlag(KSMInhibitorFlag flag);
    // 添加抑制器
    QSharedPointer<Inhibitor> addInhibitor(const QString &appID,
                                           uint32_t toplevelXID,
                                           const QString &reason,
                                           uint32_t flags,
                                           const QString &startupID = QString());

    // 删除抑制器
    void deleteInhibitor(uint32_t cookie);
    void deleteInhibitorByStartupID(const QString &startupID);
    void deleteInhibitorsWithStartupID();
    // 存在指定类型的抑制器
    bool hasInhibitor(uint32_t flags);

Q_SIGNALS:
    void inhibitorAdded(QSharedPointer<Inhibitor> inhibitor);
    void inhibitorDeleted(QSharedPointer<Inhibitor> inhibitor);

private:
    void init();
    bool addInhibitor(QSharedPointer<Inhibitor> inhibitor);
    void updatePresence();

private:
    static InhibitorManager *m_instance;

    Presence *m_presence;
    // 抑制器 <cookie, Inhibitor>
    QMap<uint32_t, QSharedPointer<Inhibitor>> m_inhibitors;
};
}  // namespace Kiran