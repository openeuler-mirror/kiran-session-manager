/**
 * Copyright (c) 2020 ~ 2021 KylinSec Co., Ltd.
 * kiran-session-manager is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for the full details.
 */

#ifndef KIRAN_SESSION_MANAGER_UI_SCREEN_MANAGER_H
#define KIRAN_SESSION_MANAGER_UI_SCREEN_MANAGER_H

#include <QMap>
#include <QObject>
#include <QPointer>

class QScreen;

namespace Kiran
{
class BackgroundWindow;
class ExitQueryWindow;

// 根据 QScreen 生成并管理 BackgroundWindow，ExitQueryWindow 随光标所在屏动态切换
// 在 Background 上感知 enter，动态更新
class ScreenManager : public QObject
{
    Q_OBJECT
public:
    explicit ScreenManager(int32_t powerAction, QObject *parent = nullptr);
    ~ScreenManager() override;

    void show();

private:
    // 为当前所有屏幕建立 QScreen -> BackgroundWindow 映射（仅用于初始化
    void buildInitialBackgroundWindows();
    // 若该 Screen 尚无 BackgroundWindow 则创建并加入映射
    void ensureBackgroundForScreen(QScreen *screen);
    // 初始放置：创建 ExitQueryWindow 并放到光标所在屏（或主屏），后续由 cursorEntered 驱动
    void placeExitQueryWindowOnCursorScreen();
    // 将 ExitQueryWindow 挂到指定 Background 上并铺满，由 BackgroundWindow::cursorEntered 调用
    void moveExitQueryWindowToBackground(BackgroundWindow *bw);
    BackgroundWindow *backgroundForScreen(QScreen *screen) const;
    // 取任意一个可用的 BackgroundWindow（用于屏幕移除时迁移 ExitQueryWindow）
    BackgroundWindow *anyAvailableBackground() const;

private Q_SLOTS:
    void onScreenAdded(QScreen *screen);
    void onScreenRemoved(QScreen *screen);

private:
    int32_t m_powerAction;
    QMap<QScreen *, QPointer<BackgroundWindow>> m_screenToBackground;
    QPointer<ExitQueryWindow> m_exitQueryWindow;
};

}  // namespace Kiran

#endif  // KIRAN_SESSION_MANAGER_UI_SCREEN_MANAGER_H
