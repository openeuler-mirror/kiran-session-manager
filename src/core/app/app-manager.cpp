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

#include "src/core/app/app-manager.h"
#include <KConfig>
#include <KConfigGroup>
#include <QDir>
#include <QTimer>
#include <fstream>
#include "lib/base/base.h"
#include "src/core/app/app.h"
#include "src/core/utils.h"

namespace Kiran
{
#define BLACKLIST_APPS_PATH KSM_INSTALL_DATADIR "/blacklist_autostart_apps.txt"
#define SESSIONS_DIR KSM_INSTALL_DATADIR "/sessions"
#define SESSION_GROUP_NAME "Kiran Session"
#define SESSION_KEY_REQUIRED_COMPONENTS "RequiredComponents"

AppManager::AppManager(const QString &sessionName) : QObject(nullptr),
                                                     m_sessionName(sessionName)
{
}

AppManager *AppManager::m_instance = nullptr;
void AppManager::globalInit(const QString &sessionName)
{
    m_instance = new AppManager(sessionName);
    m_instance->init();
}

App *AppManager::getAppByStartupID(const QString &startupID)
{
    KLOG_DEBUG() << "Startup id: " << startupID;

    for (auto iter : this->m_apps)
    {
        if (iter->getStartupID() == startupID)
        {
            return iter;
        }
    }
    return nullptr;
}

QList<App *> AppManager::startApps(int32_t phase)
{
    QList<App *> apps;

    for (auto app : this->getAppsByPhase(phase))
    {
        if (!app->canLaunched())
        {
            KLOG_DEBUG() << "The app " << app->getAppID() << " cannot be launched.";
            continue;
        }

        // 如果应用由设置延时执行，则添加定时器延时启动应用
        auto delay = app->getDelay() * 1000;

        /* 由于kiran-session-daemon和mate-session-daemon的部分插件不能同时启动，
           因此需要等到kiran-session-daemon启动并调用RegisterClient接口后才能启动mate-settings-daemon，
           在此阶段kiran-session-daemon会调用gsettings把与mate-session-daemon冲突的插件关闭掉，
           当回话管理收到kiran-session-daemon的RegisterClient调用后再启动mate-settings-daemon*/
        KLOG_DEBUG() << "DESKTOP ID: " << app->getAppID();
        if (app->getAppID() == "mate-settings-daemon.desktop")
        {
            KLOG_DEBUG() << "The boot of mate-settings-daemon need be delayed until the kiran-session-daemon calls RegisterClient.";
            apps.push_back(app);
            continue;
        }

        if (delay > 0)
        {
            QTimer::singleShot(delay, [app]()
                               {
                                   if (app->canLaunched())
                                   {
                                       app->start();
                                   }
                               });
            KLOG_DEBUG() << "The app " << app->getAppID() << " is scheduled to start after " << delay << " milliseconds.";
        }
        else
        {
            CONTINUE_IF_FALSE(app->start());
            KLOG_DEBUG() << "The app " << app->getAppID() << " is started.";
        }
        apps.push_back(app);
    }
    return apps;
}

void AppManager::init()
{
    this->loadApps();
}

void AppManager::loadApps()
{
    this->loadBlacklistAutostartApps();
    this->loadAutostartApps();
    this->loadRequiredApps();
}

void AppManager::loadRequiredApps()
{
    auto sessionFilePath = QString("%1/%2.session").arg(SESSIONS_DIR).arg(this->m_sessionName);

    KConfig keyFile(sessionFilePath, KConfig::SimpleConfig);

    auto sessionGroup = keyFile.group(SESSION_GROUP_NAME);
    auto requiredComponents = sessionGroup.readXdgListEntry(SESSION_KEY_REQUIRED_COMPONENTS);

    KLOG_DEBUG() << "Load required components: " << requiredComponents.join(",") << " from " << sessionFilePath;

    for (auto &iter : requiredComponents)
    {
        this->addApp(iter + ".desktop");
    }
}

void AppManager::loadBlacklistAutostartApps()
{
    static const int DESKTOP_ID_MAX_LEN = 256;
    this->m_blacklistApps.clear();

    std::ifstream ifs(BLACKLIST_APPS_PATH, std::ifstream::in);
    if (ifs.is_open())
    {
        while (ifs.good())
        {
            char desktop_id[DESKTOP_ID_MAX_LEN] = {0};
            ifs.getline(desktop_id, DESKTOP_ID_MAX_LEN);
            this->m_blacklistApps.insert(desktop_id);
        }
    }
}

void AppManager::loadAutostartApps()
{
    for (auto autostartDir : Utils::getDefault()->getAutostartDirs())
    {
        QDir dir(autostartDir);

        KLOG_DEBUG() << "Load autostart app from " << autostartDir;

        for (auto &fileName : dir.entryList(QDir::Files))
        {
            auto filePath = QString("%1/%2").arg(autostartDir).arg(fileName);

            if (!filePath.endsWith(".desktop"))
            {
                continue;
            }

            if (this->m_blacklistApps.find(fileName) != this->m_blacklistApps.end())
            {
                KLOG_DEBUG() << "The app " << fileName << " is in black list, so it isn't loaded.";
            }
            else
            {
                this->addApp(filePath);
            }
        }
    }
}

bool AppManager::addApp(const QString &fileName)
{
    auto app = new App(fileName, this);
    auto appID = app->getAppID();
    if (this->m_apps.find(appID) != this->m_apps.end())
    {
        KLOG_DEBUG() << "The app " << appID << " already exist.";
        delete app;
        return false;
    }
    KLOG_DEBUG() << "Add app " << appID << " with location " << fileName << " to AppManager.";

    this->m_apps.insert(appID, app);
    connect(app, &App::AppExited, [this, app]()
            { emit this->AppExited(app); });
    return true;
}

QList<App *> AppManager::getAppsByPhase(int32_t phase)
{
    QList<App *> apps;
    for (auto iter : this->m_apps)
    {
        if (iter->getPhase() == phase)
        {
            apps.push_back(iter);
        }
    }
    return apps;
}

}  // namespace Kiran
