/**
 * Copyright (c) 2025 ~ 2026 KylinSec Co., Ltd.
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

#include <signal.h>
#include <QtTest>

class QProcess;
class QGSettings;
class QElapsedTimer;

namespace Kiran
{
class TestBenchmark : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    // 测试会话启动时间
    void sessionBootTime();

private:
    static void startXorgProcessFinished(int signum, siginfo_t* info, void* data);

private:
    QGSettings* m_settings;
    QProcess* m_xorgProcess;
    QProcess* m_dbusSessionProcess;
    QElapsedTimer m_sessionElapsedTimer;

    static bool m_xorgProcessStarted;
};

}  // namespace Kiran