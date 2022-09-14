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

#include <QString>
#include <cstdint>
#include "ksm-error-i.h"

namespace Kiran
{
#define KSM_ERROR2STR(error_code) KSMError::getErrorDesc(error_code)

#define DBUS_ERROR_REPLY(type, error_code, ...)                                                        \
    {                                                                                                  \
        auto errMessage = QString::asprintf(KSM_ERROR2STR(error_code).toUtf8().data(), ##__VA_ARGS__); \
        sendErrorReply(type, errMessage);                                                              \
    }

#define DBUS_ERROR_REPLY_AND_RET(type, error_code, ...) \
    DBUS_ERROR_REPLY(type, error_code, ##__VA_ARGS__);  \
    return;

class KSMError
{
public:
    KSMError();
    virtual ~KSMError(){};

    static QString getErrorDesc(KSMErrorCode error_code);
};

}  // namespace Kiran