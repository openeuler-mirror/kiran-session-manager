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

#include "exit-query-window.h"
#include <KDesktopFile>
#include <QApplication>
#include <QDesktopWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPainter>
#include <QResizeEvent>
#include <QScreen>
#include <QTimer>
#include <iostream>
#include <kiran-push-button.h>
#include "inhibitor-row.h"
#include "lib/base/base.h"
#include "session_manager_interface.h"
#include "ui_exit-query-window.h"

QT_BEGIN_NAMESPACE
Q_WIDGETS_EXPORT void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed = 0);
QT_END_NAMESPACE

namespace Kiran
{
#define BLUR_RADIUS 10

ExitQueryWindow::ExitQueryWindow(int32_t powerAction,
                                 QWidget *parent) : QWidget(parent, Qt::FramelessWindowHint | Qt::BypassWindowManagerHint | Qt::WindowStaysOnTopHint | Qt::Widget),
                                                    m_ui(new Ui::ExitQueryWindow),
                                                    m_powerAction(powerAction)
{
    this->m_ui->setupUi(this);

    this->m_sessionManagerProxy = new SessionManagerProxy(KSM_DBUS_NAME,
                                                          KSM_DBUS_OBJECT_PATH,
                                                          QDBusConnection::sessionBus(),
                                                          this);

    this->initUI();
}

void ExitQueryWindow::initUI()
{
    auto primaryScreen = QApplication::primaryScreen();
    auto desktopRect = primaryScreen->virtualGeometry();

    this->m_ui->m_inhibitorsScrollArea->setStyleSheet("QScrollArea {background-color:transparent;}");
    this->m_ui->m_inhibitorsScrollArea->setFrameStyle(QFrame::NoFrame);
    this->m_ui->m_inhibitorsScrollArea->viewport()->setStyleSheet("background-color:transparent;");
    KiranPushButton::setButtonType(this->m_ui->m_ok, KiranPushButton::BUTTON_Default);

    this->initInhibitors();
    this->onVirtualGeometryChanged(desktopRect);

    connect(primaryScreen, SIGNAL(virtualGeometryChanged(const QRect &)), this, SLOT(onVirtualGeometryChanged(const QRect &)));

    connect(this->m_ui->m_ok, &QPushButton::clicked, [this](bool)
            { this->quit("ok"); });

    connect(this->m_ui->m_cancel, &QPushButton::clicked, [this](bool)
            { this->quit("cancel"); });
}

void ExitQueryWindow::initInhibitors()
{
    QJsonParseError jsonError;

    auto reply = this->m_sessionManagerProxy->GetInhibitors();
    reply.waitForFinished();

    if (reply.isError())
    {
        KLOG_WARNING() << "Failed to get inhibitors: " << reply.error().message();
        return;
    }

    auto inhibitors = reply.value();

    auto jsonDoc = QJsonDocument::fromJson(inhibitors.toUtf8(), &jsonError);
    if (jsonDoc.isNull())
    {
        KLOG_WARNING() << "Parser inhibitors failed: " << jsonError.errorString();
        return;
    }

    auto jsonRoot = jsonDoc.array();
    this->m_ui->m_titleDesc->setText(tr("If you want to go back and save your work, click 'cancel' and finish what you want to do"));

    /* 大多数情况是不会出现为0的情况。出现这种情况的原因是在启动当前进程的过程中，会话管理进程收到了客户端响应信息并删除掉了客户端的抑制器，导致此时获取的数量为0。
       主要原因是因为在启动当前进程时有一个延时差，因此这里再做一次判断，如果抑制器数量为0，就之前退出当前进程了。*/

    switch (this->m_powerAction)
    {
    case PowerAction::POWER_ACTION_LOGOUT:
    {
        this->m_ui->m_ok->setText(tr("Logout"));
        this->m_ui->m_title->setText(tr("The current user is being logged out"));
        break;
    }
    case PowerAction::POWER_ACTION_SHUTDOWN:
    {
        this->m_ui->m_ok->setText(tr("Shutdown"));
        this->m_ui->m_title->setText(tr("Shutting down the system"));
        break;
    }
    case PowerAction::POWER_ACTION_REBOOT:
    {
        this->m_ui->m_ok->setText(tr("Reboot"));
        this->m_ui->m_title->setText(tr("Restarting the system"));
        break;
    }
    default:
    {
        KLOG_WARNING() << "The power action is unsupported. action: " << this->m_powerAction;
        break;
    }
    }
    if (jsonRoot.size() == 0)
    {
        this->m_ui->m_inhibitorsScrollArea->hide();
        this->m_ui->m_content->setFixedSize(400, 180);
    }
    else
    {
        this->m_ui->m_content->setFixedSize(600, 400);
        for (auto iter : jsonRoot)
        {
            auto jsonInhibitor = iter.toObject();
            if (jsonInhibitor.contains(KSM_INHIBITOR_JK_FLAGS) &&
                jsonInhibitor.take(KSM_INHIBITOR_JK_FLAGS).toInt() == KSMInhibitorFlag::KSM_INHIBITOR_FLAG_QUIT)
            {
                this->m_ui->m_inhibitorsLayout->addWidget(new InhibitorRow(jsonInhibitor));
            }
        }

        this->m_ui->m_inhibitorsLayout->addStretch();
    }
}

void ExitQueryWindow::quit(const QString &result)
{
    QJsonObject jsonObj;
    jsonObj.insert("response_id", result);
    QJsonDocument jsonDoc(jsonObj);
    std::cout << jsonDoc.toJson().data();
    QApplication::quit();
}

// void ExitQueryWindow::resizeEvent(QResizeEvent *event)
// {
//     KLOG_WARNING() << "cur height: " << event->size().height() << " old height: " << event->oldSize().height();
//     QWidget::resizeEvent(event);
// }

void ExitQueryWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    auto screen = QApplication::primaryScreen();
    auto desktopRect = screen->virtualGeometry();

    // 只加载一次
    if (this->m_backgroundPixmap.isNull())
    {
        auto image = screen->grabWindow(QApplication::desktop()->winId(),
                                        desktopRect.x(),
                                        desktopRect.y(),
                                        desktopRect.width(),
                                        desktopRect.height())
                         .toImage();
        qt_blurImage(image, BLUR_RADIUS, true);
        this->m_backgroundPixmap = QPixmap::fromImage(image);
    }
    painter.drawPixmap(desktopRect.x(), desktopRect.y(), this->m_backgroundPixmap);

    QWidget::paintEvent(event);
}

void ExitQueryWindow::onVirtualGeometryChanged(const QRect &rect)
{
    this->move(rect.topLeft());
    this->resize(QSize(rect.width(), rect.height()));

    auto primaryScreen = QApplication::primaryScreen();
    auto primaryRect = primaryScreen->geometry();

    auto x = primaryRect.x() + (primaryRect.width() - this->m_ui->m_content->width()) / 2;
    auto y = primaryRect.y() + (primaryRect.height() - this->m_ui->m_content->height()) / 2;

    this->m_ui->m_leftSpacer->changeSize(x, 1);
    this->m_ui->m_topSpacer->changeSize(1, y);
}

}  // namespace Kiran
