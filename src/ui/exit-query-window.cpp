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
#include <kiran-push-button.h>
#include <ksm-i.h>
#include <KDesktopFile>
#include <QApplication>
#include <QDesktopWidget>
#include <QGSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPainter>
#include <QResizeEvent>
#include <QScreen>
#include <QTimer>
#include <iostream>
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
    m_countdownTimer = new QTimer(this);
    m_gsettings = new QGSettings(KSM_SCHEMA_ID);
    // 从gsettings获取倒计时时间
    m_countdownSeconds = m_gsettings->get(KSM_SCHEMA_KEY_EXIT_WINDOW_COUNTDOWN_TIMEOUT).toUInt();

    m_sessionManagerProxy = new SessionManagerProxy(KSM_DBUS_NAME,
                                                    KSM_DBUS_OBJECT_PATH,
                                                    QDBusConnection::sessionBus(),
                                                    this);

    m_ui->setupUi(this);
    initUI();
}

void ExitQueryWindow::initUI()
{
    auto primaryScreen = QApplication::primaryScreen();
    auto desktopRect = primaryScreen->virtualGeometry();

    m_ui->m_inhibitorsScrollArea->setStyleSheet("QScrollArea {background-color:transparent;}");
    m_ui->m_inhibitorsScrollArea->setFrameStyle(QFrame::NoFrame);
    m_ui->m_inhibitorsScrollArea->viewport()->setStyleSheet("background-color:transparent;");
    KiranPushButton::setButtonType(m_ui->m_ok, KiranPushButton::BUTTON_Default);

    initInhibitors();
    onVirtualGeometryChanged(desktopRect);

    connect(primaryScreen, SIGNAL(virtualGeometryChanged(const QRect &)), this, SLOT(onVirtualGeometryChanged(const QRect &)));

    connect(m_ui->m_ok, &QPushButton::clicked, [this](bool)
            { this->quit("ok"); });

    connect(m_ui->m_cancel, &QPushButton::clicked, [this](bool)
            { this->quit("cancel"); });
}

void ExitQueryWindow::initInhibitors()
{
    QJsonParseError jsonError;

    auto reply = m_sessionManagerProxy->GetInhibitors();
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
    m_ui->m_titleDesc->setText(tr("If you want to go back and save your work, click 'cancel' and finish what you want to do"));

    /* 大多数情况是不会出现为0的情况。出现这种情况的原因是在启动当前进程的过程中，会话管理进程收到了客户端响应信息并删除掉了客户端的抑制器，导致此时获取的数量为0。
       主要原因是因为在启动当前进程时有一个延时差，因此这里再做一次判断，如果抑制器数量为0，就之前退出当前进程了。*/

    switch (m_powerAction)
    {
    case PowerAction::POWER_ACTION_LOGOUT:
    {
        m_actionText = tr("Logout");
        m_ui->m_title->setText(tr("The current user is being logged out"));
        break;
    }
    case PowerAction::POWER_ACTION_SHUTDOWN:
    {
        m_actionText = tr("Shutdown");
        m_ui->m_title->setText(tr("Shutting down the system"));
        break;
    }
    case PowerAction::POWER_ACTION_REBOOT:
    {
        m_actionText = tr("Reboot");
        m_ui->m_title->setText(tr("Restarting the system"));
        break;
    }
    default:
    {
        m_actionText = tr("Unknown");
        KLOG_WARNING() << "The power action is unsupported. action: " << m_powerAction;
        break;
    }
    }
    m_ui->m_ok->setText(m_actionText);

    if (jsonRoot.size() == 0)
    {
        m_ui->m_inhibitorsScrollArea->hide();
        m_ui->m_content->setFixedSize(400, 180);
    }
    else
    {
        m_ui->m_content->setFixedSize(600, 400);
        for (auto iter : jsonRoot)
        {
            auto jsonInhibitor = iter.toObject();
            if (jsonInhibitor.contains(KSM_INHIBITOR_JK_FLAGS) &&
                jsonInhibitor.take(KSM_INHIBITOR_JK_FLAGS).toInt() == KSMInhibitorFlag::KSM_INHIBITOR_FLAG_QUIT)
            {
                m_ui->m_inhibitorsLayout->addWidget(new InhibitorRow(jsonInhibitor));
            }
        }

        m_ui->m_inhibitorsLayout->addStretch();
    }

    // 如果有配置倒计时时间，则启动倒计时
    if (m_countdownSeconds > 0)
    {
        startCountdown();
    }
}

void ExitQueryWindow::startCountdown()
{
    // 设置倒计时定时器，每秒触发一次
    connect(m_countdownTimer, &QTimer::timeout, this, &ExitQueryWindow::updateCountdown);
    m_countdownTimer->start(1000);

    // 立即更新一次显示
    updateCountdown();
}

void ExitQueryWindow::updateCountdown()
{
    if (m_countdownSeconds > 0)
    {
        // 更新按钮文本，显示倒计时
        QString buttonText = QString("%1 (%2s)").arg(m_actionText).arg(m_countdownSeconds);
        m_ui->m_ok->setText(buttonText);
        --m_countdownSeconds;
    }
    else
    {
        // 倒计时结束，自动触发退出
        m_countdownTimer->stop();
        this->quit("ok");
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

void ExitQueryWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    auto screen = QApplication::primaryScreen();
    auto desktopRect = screen->virtualGeometry();

    // 只加载一次
    if (m_backgroundPixmap.isNull())
    {
        auto image = screen->grabWindow(QApplication::desktop()->winId(),
                                        desktopRect.x(),
                                        desktopRect.y(),
                                        desktopRect.width(),
                                        desktopRect.height())
                         .toImage();
        qt_blurImage(image, BLUR_RADIUS, true);
        m_backgroundPixmap = QPixmap::fromImage(image);
    }
    painter.drawPixmap(desktopRect.x(), desktopRect.y(), m_backgroundPixmap);

    QWidget::paintEvent(event);
}

void ExitQueryWindow::onVirtualGeometryChanged(const QRect &rect)
{
    move(rect.topLeft());
    resize(QSize(rect.width(), rect.height()));

    auto primaryScreen = QApplication::primaryScreen();
    auto primaryRect = primaryScreen->geometry();

    auto x = primaryRect.x() + (primaryRect.width() - m_ui->m_content->width()) / 2;
    auto y = primaryRect.y() + (primaryRect.height() - m_ui->m_content->height()) / 2;

    m_ui->m_leftSpacer->changeSize(x, 1);
    m_ui->m_topSpacer->changeSize(1, y);
}

}  // namespace Kiran
