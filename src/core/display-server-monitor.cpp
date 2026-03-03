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
#include "display-server-monitor.h"
#include <qt5-log-i.h>
#include <QSocketNotifier>
#include <xcb/xcb.h>

namespace Kiran
{
DisplayServerMonitor::DisplayServerMonitor(DisplayServerType type,
                                           QObject* parent)
    : QObject(parent), m_type(type)
{
    initiate();
}

DisplayServerMonitor::~DisplayServerMonitor()
{
    if (m_xcbConnection)
    {
        xcb_disconnect(m_xcbConnection);
        m_xcbConnection = nullptr;
    }
}

void DisplayServerMonitor::initiate()
{
    bool res = false;
    switch (m_type)
    {
    case X11:
        res = initiateX11Connection();
        break;
    case WAYLAND:
    default:
        KLOG_WARNING() << "Unknown display server type";
        break;
    }

    if (!res)
    {
        KLOG_ERROR() << "Failed to initiate display server monitor";
        exit(1);
    }
}

bool DisplayServerMonitor::initiateX11Connection()
{
    const auto env = qgetenv("DISPLAY");
    m_xcbConnection = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(m_xcbConnection))
    {
        KLOG_ERROR() << "Failed to connect to X server" << env.data();
        return false;
    }

    int fd = xcb_get_file_descriptor(m_xcbConnection);
    auto readNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(readNotifier,
            &QSocketNotifier::activated,
            this,
            &DisplayServerMonitor::checkDisplayServer);

    return true;
}

void DisplayServerMonitor::checkDisplayServer()
{
    auto bRes = false;
    switch (m_type)
    {
    case X11:
        bRes = checkX11DisplayServer();
        break;
    case WAYLAND:
    default:
        KLOG_WARNING() << "Unknown display server type";
        break;
    }

    if (!bRes)
    {
        KLOG_WARNING() << "Display server has died";
        exit(1);
    }
}

bool DisplayServerMonitor::checkX11DisplayServer()
{
    if (!m_xcbConnection)
    {
        KLOG_WARNING() << "X11 connection is not established";
        return false;
    }

    int xcbErr = xcb_connection_has_error(m_xcbConnection);
    if (xcbErr)
    {
        KLOG_WARNING() << "X11 connection has error:" << xcbErr << ", Did the X11 server die?";
        return false;
    }

    // 读出所有缓冲区事件
    xcb_generic_event_t* event = nullptr;
    while ((event = xcb_poll_for_event(m_xcbConnection)) != nullptr)
    {
        free(event);
    }

    return true;
}
}  // namespace Kiran