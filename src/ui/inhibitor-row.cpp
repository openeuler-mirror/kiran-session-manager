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

#include "inhibitor-row.h"
#include <KDesktopFile>
#include <QScopedPointer>
#include "lib/base/base.h"
#include "ui_inhibitor-row.h"

namespace Kiran
{
InhibitorRow::InhibitorRow(const QJsonObject &inhibitor,
                           QWidget *parent) : QWidget(parent),
                                              m_ui(new Ui::InhibitorRow),
                                              m_inhibitor(inhibitor)
{
    m_ui->setupUi(this);
    initUI();
}

void InhibitorRow::initUI()
{
    auto appID = m_inhibitor.value(KSM_INHIBITOR_JK_APP_ID).toString();
    auto reason = m_inhibitor.value(KSM_INHIBITOR_JK_REASON).toString();

    KLOG_DEBUG() << "Init inhibitor row for app: " << appID;

    QScopedPointer<KDesktopFile> desktopFile(new KDesktopFile(appID));

    if (desktopFile)
    {
        auto appIcon = QIcon::fromTheme(desktopFile->readIcon());
        if (appIcon.isNull())
        {
            m_ui->m_appIcon->setIcon(QIcon(":/app-missing.svg"));
        }
        else
        {
            m_ui->m_appIcon->setIcon(appIcon);
        }
        m_ui->m_appName->setText(desktopFile->readName());
    }
    else
    {
        m_ui->m_appIcon->setIcon(QIcon(":/app-missing.svg"));
        m_ui->m_appName->setText(tr("Unknown application"));
    }
    m_ui->m_appDesc->setText(reason);
}

}  // namespace Kiran
