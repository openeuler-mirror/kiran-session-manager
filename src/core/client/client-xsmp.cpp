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

#include "src/core/client/client-xsmp.h"
#include "lib/base/base.h"

#include <X11/SM/SMlib.h>

#include <QFileInfo>

namespace Kiran
{
ClientXsmp::ClientXsmp(const QString &startupID,
                       SmsConn sms_connection,
                       QObject *parent) : Client(startupID, parent),
                                          m_smsConnection(sms_connection),
                                          m_currentSaveYourself(-1),
                                          m_nextSaveYourself(-1),
                                          m_nextSaveYourselfAllowInteract(false)
{
}

ClientXsmp::~ClientXsmp()
{
    for (auto props : m_props)
    {
        SmFreeProperty(static_cast<SmProp *>(props));
    }
}

QString ClientXsmp::getAppID()
{
    auto appID = Client::getAppID();
    RETURN_VAL_IF_TRUE(appID.length() > 0, appID);

    return QFileInfo(getProgramName()).fileName() + ".desktop";
}

bool ClientXsmp::cancelEndSession()
{
    SmsShutdownCancelled(m_smsConnection);
    return true;
}

bool ClientXsmp::queryEndSession(bool interact)
{
    /* SmsConn/SaveType/shutdown/interact style/fast */
    SmsSaveYourself(m_smsConnection,
                    SmSaveGlobal,
                    True,
                    interact ? SmInteractStyleAny : SmInteractStyleNone,
                    False);

    return true;
}

bool ClientXsmp::endSession(bool saveData)
{
    SmsSaveYourself(m_smsConnection,
                    saveData ? SmSaveBoth : SmSaveGlobal,
                    True,
                    SmInteractStyleNone,
                    True);
    return true;
}

bool ClientXsmp::endSessionPhase2()
{
    SmsSaveYourselfPhase2(m_smsConnection);
    return true;
}

bool ClientXsmp::stop()
{
    SmsDie(m_smsConnection);
    return true;
}

void ClientXsmp::updateProperty(void *property)
{
    RETURN_IF_FALSE(property != NULL);
    SmProp *smProperty = static_cast<SmProp *>(property);
    KLOG_DEBUG("property name: %s.", smProperty->name);

    deleteProperty(POINTER_TO_STRING(smProperty->name));
    m_props.push_back(property);

    switch (shash(smProperty->name))
    {
    case CONNECT(SmDiscardCommand, _hash):
    {
        auto discardCommand = propToCommand(smProperty);
        KLOG_DEBUG() << "Property value: " << discardCommand;
        break;
    }
    case CONNECT(SmProcessID, _hash):
    {
        auto procesID = POINTER_TO_STRING(static_cast<const char *>(smProperty->vals[0].value)).toULong();
        KLOG_DEBUG() << "Property value: " << procesID;
        break;
    }
    case CONNECT(SmProgram, _hash):
    {
        auto programName = QString::fromUtf8(static_cast<const char *>(smProperty->vals[0].value), smProperty->vals[0].length);
        KLOG_DEBUG() << "Property value: " << programName;
        break;
    }
    case CONNECT(SmRestartCommand, _hash):
    {
        auto restartCommand = propToCommand(smProperty);
        KLOG_DEBUG() << "Property value: " << restartCommand;
        break;
    }
    case CONNECT(SmRestartStyleHint, _hash):
    {
        auto restartStyleHint = (static_cast<unsigned char *>(smProperty->vals[0].value))[0];
        KLOG_DEBUG() << "Property value: " << restartStyleHint;
        break;
    }
    default:
        break;
    }
}

void ClientXsmp::deleteProperty(const QString &propertyName)
{
    KLOG_DEBUG() << "Delete Property " << propertyName;

    auto index = getPropertyIndex(propertyName);
    if (index >= 0)
    {
        SmFreeProperty(static_cast<SmProp *>(m_props[index]));
        m_props.remove(index);
    }
}

void *ClientXsmp::getProperty(const QString &propertyName)
{
    auto index = getPropertyIndex(propertyName);
    RETURN_VAL_IF_TRUE(index < 0, NULL);
    return m_props[index];
}

QString ClientXsmp::getProgramName()
{
    auto property = getProperty(SmProgram);
    RETURN_VAL_IF_TRUE(property == NULL, QString());
    SmProp *sm_property = static_cast<SmProp *>(property);
    return QString::fromUtf8(static_cast<const char *>(sm_property->vals[0].value), sm_property->vals[0].length);
}

int32_t ClientXsmp::getPropertyIndex(const QString &propertyName)
{
    for (int32_t i = 0; i < m_props.size(); ++i)
    {
        auto prop = static_cast<SmProp *>(m_props[i]);

        RETURN_VAL_IF_TRUE(propertyName == POINTER_TO_STRING(prop->name), i);
    }
    return -1;
}

QString ClientXsmp::propToCommand(void *property)
{
    QString retval;
    SmProp *sm_property = static_cast<SmProp *>(property);

    for (int32_t i = 0; i < sm_property->num_vals; i++)
    {
        const char *str = static_cast<char *>(sm_property->vals[i].value);
        int32_t strlen = sm_property->vals[i].length;

        auto need_quotes = false;
        for (int32_t j = 0; j < strlen; j++)
        {
            if (!isalnum(str[j]) && !strchr("-_=:./", str[j]))
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
            retval += QString::fromUtf8(str, strlen);
        }
        else
        {
            retval.push_back('\'');
            while (str < static_cast<char *>(sm_property->vals[i].value) + strlen)
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

}  // namespace Kiran
