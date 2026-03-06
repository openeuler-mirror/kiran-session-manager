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
#pragma once
#include <QWidget>

class QScreen;

namespace Kiran
{
// 单个 QScreen 的模糊背景窗口，覆盖整块屏幕并绘制该屏的模糊截图。
class BackgroundWindow : public QWidget
{
    Q_OBJECT
public:
    explicit BackgroundWindow(QScreen *screen, QWidget *parent = nullptr);
    ~BackgroundWindow() override = default;
    QScreen *screen() const { return m_screen; }

signals:
    void resized();
    void cursorEntered();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void enterEvent(QEvent *event) override;

private:
    void updateGeometryFromScreen();
    void refreshBackground();

private Q_SLOTS:
    void onScreenGeometryChanged(const QRect &geometry);

private:
    QScreen *m_screen;
    QPixmap m_backgroundPixmap;
};

}  // namespace Kiran