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
#include "background-window.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScreen>
#include <QtWidgets>
#include <qt5-log-i.h>

QT_BEGIN_NAMESPACE
// 重载1：就地模糊
Q_WIDGETS_EXPORT void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed = 0);
// 重载2：模糊后通过 QPainter 绘制（内部按 devicePixelRatio 计算目标区域，高 DPI 更合适）
Q_WIDGETS_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

#define BLUR_RADIUS 10

namespace Kiran
{
BackgroundWindow::BackgroundWindow(QScreen *screen, QWidget *parent)
    : QWidget(parent),
      m_screen(screen)
{
    if (!m_screen)
        return;

    Qt::WindowFlags flags = Qt::WindowStaysOnTopHint | Qt::Widget
                            | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint;
    setWindowFlags(flags);
    setAttribute(Qt::WA_TranslucentBackground, false);

    updateGeometryFromScreen();
    refreshBackground();

    connect(m_screen, &QScreen::geometryChanged, this, &BackgroundWindow::onScreenGeometryChanged);
}

void BackgroundWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPixmap(0, 0, m_backgroundPixmap);
    QWidget::paintEvent(event);
}

void BackgroundWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    emit resized();
}

void BackgroundWindow::enterEvent(QEvent *event)
{
    QWidget::enterEvent(event);
    emit cursorEntered();
}

void BackgroundWindow::updateGeometryFromScreen()
{
    if (!m_screen)
        return;
    const QRect rect = m_screen->geometry();
    setGeometry(rect);
}

/*
  获取模糊背景图。
  由于实现为直接提取截图，避免截图带入自身窗口，背景窗口显示后续将不再更新。
  若后续屏幕大小发生变化，模糊背景图可能无法对应。
  需考虑更优方法
*/
void BackgroundWindow::refreshBackground()
{
    if (!m_screen)
        return;

    const QRect rect = m_screen->geometry();
    const qreal dpr = m_screen->devicePixelRatio();

    /* 
       双1080p显示器,配置拓展屏, 启动QT_SCALE_FACTOR=2 qtdiag
       # 0 "HDMI-0" Depth: 24 Primary: yes
         Geometry: 960x540+1920+0 (native: 1920x1080+1920+0) Available: 960x540+1920+0
         Virtual geometry: 2880x540+0+0 Available: 2880x540+0+0
       # 1 "VGA-0" Depth: 24 Primary: no
         Geometry: 960x540+0+0 (native: 1920x1080+0+0) Available: 960x540+0+0
         Virtual geometry: 2880x540+0+0 Available: 2880x540+0+0 

       1. 可见上述HDMI-0以及VGA，Qscreen Geometry中Size已转换为内部逻辑像素(设备像素/系数)，但是偏移量(1920)仍为物理像素。
       2. 另可见qtbase流程, 可见Size经过逻辑像素转换，但topLeft沿用native
          - src/gui/kernel/qscreen_p.h: QScreenPrivate::updateHighDpi
          - src/gui/kernel/qplatformscreen.cpp: QPlatformScreen::deviceIndependentGeometry
       3. 由于QScreen::grabWindow入参LeftTop、Size均会经过QHiDpi::toNative
          由逻辑像素转为设备像素后调用至QPlatfomScreen/QXcbScreen::grabWindow

       QScreen topleft坐标不经过系数转换大概率不为bug,考虑为保证多屏混合缩放，屏幕相对摆放不因scale和漂移
       所以此处需保证ScreenGeometry的LeftTop以及Size均已转换为设备像素，避免截图时出现偏移。
    */
    const int grabX = qRound(rect.x()/dpr);
    const int grabY = qRound(rect.y()/dpr);
    const int grabW = rect.width();
    const int grabH = rect.height();

    QPixmap grabbedPixmap = m_screen->grabWindow(QApplication::desktop()->winId(),
                                                grabX,
                                                grabY,
                                                grabW,
                                                grabH);
    if (grabbedPixmap.isNull())
    {
        KLOG_WARNING() << "failed to get window background via grabWindow";
        return;
    }

    QPixmap backgroundPixmap(grabW*dpr,grabH*dpr);
    backgroundPixmap.setDevicePixelRatio(dpr);

    QImage backgroundImage = grabbedPixmap.toImage();
    
    QPainter painter(&backgroundPixmap);
    qt_blurImage(&painter, backgroundImage, BLUR_RADIUS, true, false);
    painter.end();

    m_backgroundPixmap = backgroundPixmap;
    KLOG_INFO() << "captured background pixmap size: " << m_backgroundPixmap.size() 
                << "dpr: " << m_backgroundPixmap.devicePixelRatio();
}

void BackgroundWindow::onScreenGeometryChanged(const QRect &geometry)
{
    Q_UNUSED(geometry);
    updateGeometryFromScreen();
    update();
}
}  // namespace Kiran