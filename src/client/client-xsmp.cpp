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

#include "src/client/client-xsmp.h"

namespace Kiran
{
namespace Daemon
{
ClientXsmp::ClientXsmp(const std::string &startup_id,
                       SmsConn sms_connection) : Client(startup_id),
                                                 sms_connection_(sms_connection),
                                                 current_save_yourself_(-1),
                                                 next_save_yourself_(-1),
                                                 next_save_yourself_allow_interact_(false)
{
    this->props_ = g_ptr_array_new();
}

ClientXsmp::~ClientXsmp()
{
    if (this->props_)
    {
        g_ptr_array_foreach(this->props_, (GFunc)SmFreeProperty, NULL);
        g_ptr_array_free(this->props_, TRUE);
    }
}

std::string ClientXsmp::get_app_id()
{
    auto app_id = this->Client::get_app_id();
    RETURN_VAL_IF_TRUE(!app_id.empty(), app_id);
    return this->get_program_name() + ".desktop";
}

bool ClientXsmp::cancel_end_session()
{
    SmsShutdownCancelled(this->sms_connection_);
    return true;
}

bool ClientXsmp::query_end_session(bool interact)
{
        /* SmsConn/SaveType/shutdown/interact style/fast */
    SmsSaveYourself(this->sms_connection_,
                    SmSaveGlobal,
                    True,
                    interact ? SmInteractStyleAny : SmInteractStyleNone,
                    False);

    return true;
}

bool ClientXsmp::end_session(bool save_data)
{
    SmsSaveYourself(this->sms_connection_,
                    save_data ? SmSaveBoth : SmSaveGlobal,
                    True,
                    SmInteractStyleNone,
                    True);
    return true;
}

bool ClientXsmp::end_session_phase2()
{
    SmsSaveYourselfPhase2(this->sms_connection_);
    return true;
}

bool ClientXsmp::stop()
{
    SmsDie(this->sms_connection_);
    return true;
}

void ClientXsmp::update_property(SmProp *property)
{
    RETURN_IF_FALSE(property != NULL);
    KLOG_DEBUG("property name: %s.", property->name);

    this->delete_property(POINTER_TO_STRING(property->name));
    g_ptr_array_add(this->props_, property);

    switch (shash(property->name))
    {
    case CONNECT(SmDiscardCommand, _hash):
    {
        auto discard_command = this->prop_to_command(property);
        KLOG_DEBUG("property value: %s.", discard_command.c_str());
        break;
    }
    case CONNECT(SmProcessID, _hash):
    {
        auto process_id = std::strtoul(POINTER_TO_STRING((const char *)property->vals[0].value).c_str(), NULL, 0);
        KLOG_DEBUG("property value: %d.", process_id);
        break;
    }
    case CONNECT(SmProgram, _hash):
    {
        auto program_name = std::string((const char *)property->vals[0].value, property->vals[0].length);
        KLOG_DEBUG("property value: %s.", program_name.c_str());
        break;
    }
    case CONNECT(SmRestartCommand, _hash):
    {
        auto restart_command = this->prop_to_command(property);
        KLOG_DEBUG("property value: %s.", restart_command.c_str());
        break;
    }
    case CONNECT(SmRestartStyleHint, _hash):
    {
        auto restart_style_hint = ((unsigned char *)property->vals[0].value)[0];
        KLOG_DEBUG("property value: %d.", restart_style_hint);
        break;
    }
    default:
        break;
    }
}

void ClientXsmp::delete_property(const std::string &property_name)
{
    KLOG_DEBUG("property name: %s.", property_name.c_str());

    auto index = this->get_property_index(property_name);
    if (index >= 0)
    {
        auto prop = (SmProp *)g_ptr_array_remove_index_fast(this->props_, index);
        SmFreeProperty(prop);
    }

    // switch (shash(property_name.c_str()))
    // {
    // case CONNECT(SmDiscardCommand, _hash):
    //     this->props_.discard_command.clear();
    //     break;
    // case CONNECT(SmProcessID, _hash):
    //     this->props_.process_id = -1;
    //     break;
    // case CONNECT(SmProgram, _hash):
    //     this->props_.program.clear();
    //     break;
    // case CONNECT(SmRestartCommand, _hash):
    //     this->props_.restart_command.clear();
    //     break;
    // case CONNECT(SmRestartStyleHint, _hash):
    //     this->props_.restart_style_hint = -1;
    //     break;
    // default:
    //     break;
    // }
}

SmProp *ClientXsmp::get_property(const std::string &property_name)
{
    auto index = this->get_property_index(property_name);
    RETURN_VAL_IF_TRUE(index < 0, NULL);
    return (SmProp *)(g_ptr_array_index(this->props_, index));
}

std::string ClientXsmp::get_program_name()
{
    auto property = this->get_property(SmProgram);
    RETURN_VAL_IF_TRUE(property == NULL, std::string());
    return std::string((const char *)property->vals[0].value, property->vals[0].length);
}

int32_t ClientXsmp::get_property_index(const std::string &property_name)
{
    for (int32_t i = 0; i < (int32_t)this->props_->len; i++)
    {
        auto prop = (SmProp *)(this->props_->pdata[i]);

        RETURN_VAL_IF_TRUE(property_name == POINTER_TO_STRING(prop->name), i);
    }
    return -1;
}

std::string ClientXsmp::prop_to_command(SmProp *property)
{
    std::string retval;

    for (int32_t i = 0; i < property->num_vals; i++)
    {
        const char *str = (char *)property->vals[i].value;
        int32_t strlen = property->vals[i].length;

        auto need_quotes = false;
        for (int32_t j = 0; j < strlen; j++)
        {
            if (!g_ascii_isalnum(str[j]) && !strchr("-_=:./", str[j]))
            {
                need_quotes = true;
                break;
            }
        }

        if (i > 0)
        {
            retval.push_back(' ');
        }

        if (!need_quotes)
        {
            retval += std::string(str, strlen);
        }
        else
        {
            retval.push_back('\'');
            while (str < (char *)property->vals[i].value + strlen)
            {
                if (*str == '\'')
                {
                    retval.append("'\''");
                }
                else
                {
                    retval.push_back(*str);
                }
                str++;
            }
            retval.push_back('\'');
        }
    }

    return retval;
}

// void KSMClientXsmp::do_save_yourself(int32_t save_type, bool interact)
// {
//     if (this->next_save_yourself_ != -1)
//     {
//         KLOG_DEBUG("Skipping redundant SaveYourself for %s", this->get_id().c_str());
//         return;
//     }

//     if (this->current_save_yourself_ != -1)
//     {
//         KLOG_DEBUG("Queuing new SaveYourself for %s", this->get_id().c_str());
//         this->next_save_yourself_ = save_type;
//         this->next_save_yourself_allow_interact_ = interact;
//         return;
//     }

//     this->current_save_yourself_ = save_type;
//     this->next_save_yourself_ = -1;
//     this->next_save_yourself_allow_interact_ = false;

//     switch (save_type)
//     {
//     case SmSaveLocal:
//         SmsSaveYourself(this->sms_connection_,
//                         save_type,
//                         FALSE,
//                         SmInteractStyleNone,
//                         FALSE);
//         break;

//     default:
//         /* Logout */
//         if (!interact)
//         {
//             /* SmsConn/SaveType/shutdown/interact style/fast */
//             SmsSaveYourself(this->sms_connection_, save_type, TRUE, SmInteractStyleNone, TRUE);
//         }
//         else
//         {
//             SmsSaveYourself(this->sms_connection_, save_type, TRUE, SmInteractStyleAny, FALSE);
//         }
//         break;
//     }
// }
}  // namespace Daemon
}  // namespace Kiran
