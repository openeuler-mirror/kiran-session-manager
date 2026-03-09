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

#include <QResizeEvent>
#include <QWidget>

class SessionManagerProxy;
class QTimer;
class QGSettings;

namespace Ui
{
class ExitQueryWindow;
}  // namespace Ui

namespace Kiran
{
class BackgroundWindow;
enum ExitQueryResponse
{
    EXIT_QUERY_RESPONSE_OK,
    EXIT_QUERY_RESPONSE_CANCEL,
};

/**
 * @brief 退出确认内容窗口，仅包含对话框内容；背景与所在屏幕由 ScreenManager 管理。
 */
class ExitQueryWindow : public QWidget
{
    Q_OBJECT

public:
    ExitQueryWindow(int32_t powerAction, QWidget *parent = nullptr);
    virtual ~ExitQueryWindow(){};

    /** 将本窗口挂到指定背景窗口上并铺满，用于随光标切换屏幕时动态更新 */
    void attachToBackground(BackgroundWindow *bw);
    /** 当父背景窗口尺寸变化时，将本窗口几何更新为铺满父窗口 */
    void fitToParentBackground();

    // 获取应用/InhibitorRow数量
    int32_t getAppCount();

private:
    void initUI();
    void initInhibitors();
    void startCountdown();
    void updateCountdown();
    void quit(const QString &result);

private:
    Ui::ExitQueryWindow *m_ui;
    SessionManagerProxy *m_sessionManagerProxy;
    int32_t m_powerAction;
    QString m_actionText;
    QTimer *m_countdownTimer;
    int m_countdownSeconds;
    QGSettings *m_gsettings;
};

}  // namespace Kiran
