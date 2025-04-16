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

#include "src/core/client/client.h"
#include "src/core/app/app-manager.h"

namespace Kiran
{
Client::Client(const QString &id, QObject *parent) : QObject(parent),
                                                     m_id(id)
{
}

QString Client::getAppID()
{
    auto app = AppManager::getInstance()->getAppByStartupID(m_id);
    return app ? app->getAppID() : QString();
}
}  // namespace Kiran
