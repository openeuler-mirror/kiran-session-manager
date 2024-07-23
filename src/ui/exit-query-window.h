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

#include <QWidget>

class SessionManagerProxy;

namespace Ui
{
class ExitQueryWindow;
}  // namespace Ui

namespace Kiran
{
enum ExitQueryResponse
{
    EXIT_QUERY_RESPONSE_OK,
    EXIT_QUERY_RESPONSE_CANCEL,
};

class ExitQueryWindow : public QWidget
{
    Q_OBJECT

public:
    ExitQueryWindow(int32_t powerAction, QWidget *parent = nullptr);
    virtual ~ExitQueryWindow(){};

    // 获取应用/InhibitorRow数量
    int32_t getAppCount();

private:
    void initUI();
    void initInhibitors();

    void quit(const QString &result);

    private:
    // virtual void resizeEvent(QResizeEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;

private Q_SLOTS:
    void onVirtualGeometryChanged(const QRect &rect);

private:
    Ui::ExitQueryWindow *m_ui;
    SessionManagerProxy *m_sessionManagerProxy;
    int32_t m_powerAction;
    QPixmap m_backgroundPixmap;
    };

}  // namespace Kiran
