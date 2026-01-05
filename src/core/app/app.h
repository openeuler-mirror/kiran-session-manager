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

#include <KDesktopFile>
#include <QEnableSharedFromThis>
#include <QProcess>

namespace Kiran
{

class App : public QObject
{
    Q_OBJECT

public:
    App(const QString &filePath, QObject *parent);
    virtual ~App();

    bool start();
    bool restart();
    bool stop();
    bool isRunning();
    bool canLaunched();

    // 阶段字符串和阶段枚举转换
    static int32_t phaseStr2enum(const QString &phase_str);
    static QString phaseEnum2str(int32_t phase);

    QString getAppID() { return this->m_appID; };
    int32_t getPhase() { return this->m_phase; };
    QString getStartupID() { return this->m_startupID; };
    bool getAutoRestart() { return this->m_autoRestart; };
    int32_t getDelay() { return this->m_delay; };

    // 拓展属性，用于指定应用程序启动时是否跳过注册等待阶段(运行在KSM_PHASE_APPLICATION阶段之前的自启应用)
    bool getSkipRegistrationWait() { return this->m_skipRegistrationWait; };

Q_SIGNALS:
    void AppExited();

private:
    void loadAppInfo();

private Q_SLOTS:
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString m_appID;
    int32_t m_phase;
    QString m_startupID;
    // 正常退出后是否自动重启
    bool m_autoRestart;
    // 延时运行时间
    int32_t m_delay;
    // 是否跳过注册等待阶段
    bool m_skipRegistrationWait;

    QSharedPointer<KDesktopFile> m_appInfo;
    QProcess *m_process;
};

}  // namespace Kiran
