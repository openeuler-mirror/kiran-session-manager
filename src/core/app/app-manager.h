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

#include <QMap>
#include <QObject>
#include <QSet>
#include "src/core/app/app.h"

class QGSettings;

namespace Kiran
{
class AppManager : public QObject
{
    Q_OBJECT

public:
    AppManager(const QString& session_name);
    virtual ~AppManager(){};

    static AppManager* getInstance() { return m_instance; };

    static void globalInit(const QString& session_name);

    static void globalDeinit() { delete m_instance; };

    // 获取APP
    App* getApp(const QString& appID) { return this->m_apps.value(appID, nullptr); }
    App* getAppByStartupID(const QString& startupID);

    // 启动特定阶段的app，并返回启动成功或者延时启动的app列表
    QList<App*> startApps(int32_t phase);

Q_SIGNALS:
    void AppExited(App* app);

private:
    void init();

    void loadApps();
    void loadRequiredApps();
    void loadBlacklistAutostartApps();
    void loadAutostartApps();
    // 根据desktopinfo添加app
    bool addApp(const QString& fileName);
    // 获取在特定阶段启动的app
    QList<App*> getAppsByPhase(int32_t phase);

private:
    static AppManager* m_instance;

    QString m_sessionName;
    // 需要排除的自动启动应用程序
    QSet<QString> m_blacklistApps;
    // 开机启动应用
    QMap<QString, App*> m_apps;
};
}  // namespace Kiran
