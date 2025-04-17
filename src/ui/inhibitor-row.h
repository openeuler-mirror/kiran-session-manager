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

#pragma once

#include <QJsonObject>
#include <QWidget>

namespace Ui
{
class InhibitorRow;
}  // namespace Ui

namespace Kiran
{
class InhibitorRow : public QWidget
{
    Q_OBJECT

public:
    InhibitorRow(const QJsonObject &inhibitor, QWidget *parent = nullptr);
    virtual ~InhibitorRow(){};

private:
    void initUI();

private:
    Ui::InhibitorRow *m_ui;
    QJsonObject m_inhibitor;
};

}  // namespace Kiran
