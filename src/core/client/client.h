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

#include <QObject>

namespace Kiran
{
enum ClientEndSessionFlag
{
    // 客户端在结束会话时保存必要的信息
    CLIENT_END_SESSION_FLAG_SAVE = 1 << 1,
    // 会话结束第二阶段
    CLIENT_END_SESSION_FLAG_LAST = 1 << 2
};

enum ClientType
{
    CLIENT_TYPE_XSMP,
    CLIENT_TYPE_DBUS
};

class Client : public QObject
{
    Q_OBJECT

public:
    Client(const QString &id, QObject *parent);
    virtual ~Client(){};

    QString getID() { return this->m_id; };

    virtual QString getAppID();
    void printAssociatedApp();

    virtual ClientType getType() = 0;
    virtual bool cancelEndSession() = 0;
    virtual bool queryEndSession(bool interact) = 0;
    virtual bool endSession(bool save_data) = 0;
    virtual bool endSessionPhase2() = 0;
    virtual bool stop() = 0;

private:
    QString m_id;
    // 标记是否打印了client和app的关联日志
    bool m_printAssociatedApp;
};

}  // namespace Kiran
