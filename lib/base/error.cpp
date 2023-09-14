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

#include "lib/base/error.h"

#include <fmt/format.h>
#include <QObject>

namespace Kiran
{
KSMError::KSMError()
{
}

QString KSMError::getErrorDesc(KSMErrorCode error_code)
{
    QString error_desc;
    switch (error_code)
    {
    case KSMErrorCode::ERROR_MANAGER_PHASE_CANNOT_REGISTER:
        error_desc = QObject::tr("The phase cannot be register.");
        break;
    case KSMErrorCode::ERROR_MANAGER_CLIENT_ALREADY_REGISTERED:
        error_desc = QObject::tr("The client %s already registered.");
        break;
    case KSMErrorCode::ERROR_MANAGER_GEN_UNIQUE_COOKIE_FAILED:
        error_desc = QObject::tr("Failed to generate unique cookie.");
        break;
    case KSMErrorCode::ERROR_MANAGER_INHIBITOR_NOTFOUND:
        error_desc = QObject::tr("The inhibitor is not found.");
        break;
    case KSMErrorCode::ERROR_PRESENCE_STATUS_INVALID:
        error_desc = QObject::tr("The status is invalid.");
        break;
    case KSMErrorCode::ERROR_MANAGER_POWER_ACTION_UNSUPPORTED:
        error_desc = QObject::tr("The action is not supported.");
        break;
    case KSMErrorCode::ERROR_MANAGER_PHASE_INVALID:
        error_desc = QObject::tr("Internal error.");
        break;
    default:
        error_desc = QObject::tr("Unknown error.");
        break;
    }

    // TODO: 测试
    error_desc += QString::asprintf(QObject::tr(" (error code: 0x%x)").toStdString().c_str(), int32_t(error_code));
    return error_desc;
}
}  // namespace Kiran
