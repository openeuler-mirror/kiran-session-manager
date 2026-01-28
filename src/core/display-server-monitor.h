/**
 * Copyright (c) 2020 ~ 2025 KylinSec Co., Ltd.
 * kiran-session-manager is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 *
 * Author:     liuxinhao <liuxinhao@kylinsec.com.cn>
 */
#pragma once
#include <QObject>

class QSocketNotifier;
struct xcb_connection_t;
namespace Kiran
{
class DisplayServerMonitor : public QObject
{
    Q_OBJECT
public:
    enum DisplayServerType
    {
        X11,
        WAYLAND,
        LAST
    };
    // 会话管理需要检测显示服务存活状态：在某些场景（如 XDMCP），XServer 退出后，
    // 依赖显示服务的组件会消失，但会话管理及其拉起的部分进程不依赖显示服务，
    // 可能不会被及时释放，需要监控显示服务以触发会话退出清理。
    static DisplayServerMonitor* getInstance(DisplayServerType type = X11,
                                             QObject* parent = nullptr);
    ~DisplayServerMonitor();
    DisplayServerMonitor(const DisplayServerMonitor&) = delete;
    DisplayServerMonitor& operator=(const DisplayServerMonitor&) = delete;

Q_SIGNALS:
    void displayServerDied();

private:
    DisplayServerMonitor(DisplayServerType type, QObject* parent = nullptr);
    void initiate();
    bool initiateX11Connection();
    void checkDisplayServer();
    bool checkX11DisplayServer();
    void notifyDisplayServerDied();

private:
    DisplayServerType m_type;
    xcb_connection_t* m_xcbConnection = nullptr;
    bool m_diedNotified = false;
    static DisplayServerMonitor* s_instance;
};
}  // namespace Kiran
