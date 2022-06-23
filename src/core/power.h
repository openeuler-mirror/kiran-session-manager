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

#include <QObject>
#include "ksm-i.h"

namespace Kiran
{
class Power : public QObject
{
    Q_OBJECT
public:
    Power(QObject *parent);
    virtual ~Power(){};

    void init();

    // 是否可以执行该电源行为
    bool canPowerAction(PowerAction powerAction);
    // 执行电源行为
    bool doPowerAction(PowerAction powerAction);

private:
    // 切换用户
    bool switchUser();
    // 挂起
    bool suspend();
    // 休眠
    bool hibernate();
    // 关机
    bool shutdown();
    // 重启
    bool reboot();
};
}  // namespace Kiran