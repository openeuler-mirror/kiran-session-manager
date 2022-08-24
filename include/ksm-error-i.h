/**
 * @file          /kiran-ssr-manager/home/tangjie02/git/kiran-session-manager/include/ksm-error-i.h
 * @brief         
 * @author        tangjie02 <tangjie02@kylinos.com.cn>
 * @copyright (c) 2020~2021 KylinSec Co., Ltd. All rights reserved. 
 */
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

#ifdef __cplusplus
extern "C"
{
#endif

    enum KSMErrorCode
    {
        // Common
        SUCCESS,
        ERROR_FAILED,

        // MANAGER
        ERROR_MANAGER_PHASE_CANNOT_REGISTER,
        ERROR_MANAGER_CLIENT_ALREADY_REGISTERED,
        ERROR_MANAGER_APP_NOTFOUND,
        ERROR_MANAGER_GEN_UNIQUE_COOKIE_FAILED,
        ERROR_MANAGER_INHIBITOR_NOTFOUND,
        ERROR_MANAGER_GET_INHIBITORS_FAILED,
        ERROR_MANAGER_PHASE_INVALID,
        ERROR_MANAGER_POWER_ACTION_UNSUPPORTED,

        // PRESENCE
        ERROR_PRESENCE_STATUS_INVALID,

    };
#ifdef __cplusplus
}
#endif
