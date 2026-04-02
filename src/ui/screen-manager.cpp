/**
 * Copyright (c) 2020 ~ 2026 KylinSec Co., Ltd.
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
#include "screen-manager.h"
#include "background-window.h"
#include "exit-query-window.h"
#include <QApplication>
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <QtGlobal>

namespace Kiran
{
ScreenManager::ScreenManager(int32_t powerAction, QObject *parent)
    : QObject(parent),
      m_powerAction(powerAction)
{
    connect(qApp, &QGuiApplication::screenAdded, this, &ScreenManager::onScreenAdded);
    connect(qApp, &QGuiApplication::screenRemoved, this, &ScreenManager::onScreenRemoved);
    buildInitialBackgroundWindows();
    placeExitQueryWindowOnCursorScreen();
}

ScreenManager::~ScreenManager()
{
    for (auto it = m_screenToBackground.begin(); it != m_screenToBackground.end(); ++it)
    {
        BackgroundWindow *bw = it.value().data();
        if (bw)
        {
            bw->close();
            delete bw;
        }
    }
    m_screenToBackground.clear();
}

void ScreenManager::show()
{
    for (const QPointer<BackgroundWindow> &w : m_screenToBackground)
    {
        if (w)
            w->show();
    }
    if (m_exitQueryWindow)
        m_exitQueryWindow->show();
}

void ScreenManager::buildInitialBackgroundWindows()
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens)
        ensureBackgroundForScreen(screen);
}

void ScreenManager::ensureBackgroundForScreen(QScreen *screen)
{
    if (!screen)
        return;
    if (m_screenToBackground.contains(screen) && m_screenToBackground.value(screen))
        return;
    BackgroundWindow *bw = new BackgroundWindow(screen, nullptr);
    connect(bw, &BackgroundWindow::resized, this, [this, bw]() {
        if (m_exitQueryWindow && m_exitQueryWindow->parent() == bw)
            m_exitQueryWindow->fitToParentBackground();
    });
    connect(bw, &BackgroundWindow::cursorEntered, this, [this, bw]() {
        moveExitQueryWindowToBackground(bw);
    });
    m_screenToBackground.insert(screen, bw);
    bw->show();
}

void ScreenManager::placeExitQueryWindowOnCursorScreen()
{
    const QPoint cursorPos = QCursor::pos();
    QScreen *target = nullptr;
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    target = QGuiApplication::screenAt(cursorPos);
#else
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens)
    {
        if (screen && screen->geometry().contains(cursorPos))
        {
            target = screen;
            break;
        }
    }
#endif

    if (!target)
        target = QApplication::primaryScreen();

    if (!target)
        return;

    BackgroundWindow *targetBackground = backgroundForScreen(target);

    if (!targetBackground)
        return;

    if (!m_exitQueryWindow)
        m_exitQueryWindow = new ExitQueryWindow(m_powerAction, targetBackground);
    
    m_exitQueryWindow->attachToBackground(targetBackground);
}

void ScreenManager::moveExitQueryWindowToBackground(BackgroundWindow *bw)
{
    if (!bw || !m_exitQueryWindow)
        return;
    m_exitQueryWindow->attachToBackground(bw);
}

BackgroundWindow *ScreenManager::backgroundForScreen(QScreen *screen) const
{
    if (!screen)
        return nullptr;
    return m_screenToBackground.value(screen).data();
}

BackgroundWindow *ScreenManager::anyAvailableBackground() const
{
    for (const QPointer<BackgroundWindow> &w : m_screenToBackground)
    {
        if (w)
            return w;
    }
    return nullptr;
}

void ScreenManager::onScreenAdded(QScreen *screen)
{
    ensureBackgroundForScreen(screen);
}

void ScreenManager::onScreenRemoved(QScreen *screen)
{
    BackgroundWindow *bw = m_screenToBackground.value(screen).data();
    m_screenToBackground.remove(screen);
    if (m_exitQueryWindow && bw && m_exitQueryWindow->parent() == bw)
    {
        BackgroundWindow *other = anyAvailableBackground();
        if (other)
            m_exitQueryWindow->attachToBackground(other);
    }
    if (bw)
        bw->deleteLater();
}
}  // namespace Kiran
