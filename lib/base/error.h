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

#include <fmt/format.h>
#include <QString>
#include <cstdint>
#include "ksm-error-i.h"

namespace Kiran
{
#define KSM_ERROR2STR(errorCode) KSMError::getErrorDesc(errorCode)

#define DBUS_ERROR_REPLY(type, errorCode, ...)                                                \
    {                                                                                         \
        auto errMessage = fmt::format(KSM_ERROR2STR(errorCode).toStdString(), ##__VA_ARGS__); \
        sendErrorReply(type, QString(errMessage.c_str()));                                    \
    }

#define DBUS_ERROR_REPLY_AND_RET(type, errorCode, ...) \
    DBUS_ERROR_REPLY(type, errorCode, ##__VA_ARGS__);  \
    return;

class KSMError
{
public:
    KSMError();
    virtual ~KSMError(){};

    static QString getErrorDesc(KSMErrorCode errorCode);
};

}  // namespace Kiran