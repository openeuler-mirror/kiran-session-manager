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

#include <giomm.h>
#include <cstdint>
#include "ksm-error-i.h"

namespace Kiran
{
#define KSM_ERROR2STR(error_code) KSMError::get_error_desc(error_code)

#define DBUS_ERROR_REPLY(error_code, ...)                                                    \
    {                                                                                        \
        auto err_message = fmt::format(KSM_ERROR2STR(error_code), ##__VA_ARGS__);            \
        invocation.ret(Glib::Error(G_DBUS_ERROR, G_DBUS_ERROR_FAILED, err_message.c_str())); \
    }

#define DBUS_ERROR_REPLY_AND_RET(error_code, ...) \
    DBUS_ERROR_REPLY(error_code, ##__VA_ARGS__);  \
    return;

class KSMError
{
public:
    KSMError();
    virtual ~KSMError(){};

    static std::string get_error_desc(KSMErrorCode error_code);
};

}  // namespace Kiran