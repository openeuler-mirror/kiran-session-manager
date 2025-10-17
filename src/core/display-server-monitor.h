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
    DisplayServerMonitor(DisplayServerType type, QObject* parent = nullptr);
    ~DisplayServerMonitor();

private:
    void initiate();
    bool initiateX11Connection();
    void checkDisplayServer();
    bool checkX11DisplayServer();

private:
    DisplayServerType m_type;
    xcb_connection_t* m_xcbConnection = nullptr;
};
}  // namespace Kiran