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

#include "src/core/client/client.h"
#include <QVector>

struct _SmsConn;
typedef struct _SmsConn *SmsConn;

namespace Kiran
{
class ClientXsmp : public Client
{
    Q_OBJECT
public:
    ClientXsmp(const QString &startupID, SmsConn smsConnection, QObject *parent);
    virtual ~ClientXsmp();

    ClientType getType() { return ClientType::CLIENT_TYPE_XSMP; };
    SmsConn get_sms_connection() { return this->m_smsConnection; };

    virtual QString getAppID() override;
    virtual bool cancelEndSession() override;
    virtual bool queryEndSession(bool interact) override;
    virtual bool endSession(bool save_data) override;
    virtual bool endSessionPhase2() override;
    virtual bool stop() override;

    // 添加/更新/删除/查找属性
    void updateProperty(void *property);
    void deleteProperty(const QString &property_name);
    void *getProperty(const QString &property_name);
    QVector<void *> get_properties() { return this->m_props; };

    // 获取程序名
    QString getProgramName();

signals:
    void registerRequest();
    void logoutRequest();

private:
    int32_t getPropertyIndex(const QString &property_name);
    QString propToCommand(void *prop);

private:
    SmsConn m_smsConnection;

    QString m_appID;
    uint32_t m_status;

    // XsmpClientProps props_;
    // XSMP的SmsGetPropertiesProcMask回调处理需要返回SmProp指针数组，因此这里用GPtrArray比较方便
    QVector<void *> m_props;

    int32_t m_currentSaveYourself;
    int32_t m_nextSaveYourself;
    bool m_nextSaveYourselfAllowInteract;
};
}  // namespace Kiran
