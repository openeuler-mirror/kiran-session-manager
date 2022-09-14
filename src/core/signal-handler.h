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
#include <QSet>
#include <QSocketNotifier>

namespace Kiran
{
class SignalHandler : public QObject
{
    Q_OBJECT
public:
    virtual ~SignalHandler();

    static SignalHandler *get_default();

    void addSignal(int signal);

Q_SIGNALS:
    void signalReceived(int signal);

private:
    SignalHandler();
    void handleSignal();
    static void signalHandler(int signal);

private:
    static SignalHandler *m_instance;

    QSocketNotifier *m_handler;
    QSet<int> m_signos;
    static int signalFD[2];
};
}  // namespace Kiran