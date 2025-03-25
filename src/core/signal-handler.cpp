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

#include "signal-handler.h"
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include "lib/base/base.h"

namespace Kiran
{
int SignalHandler::signalFD[2];

SignalHandler::SignalHandler() : m_handler(nullptr)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, signalFD))
    {
        KLOG_WARNING() << "Couldn't create a socketpair";
        return;
    }

    this->m_handler = new QSocketNotifier(signalFD[1], QSocketNotifier::Read, this);
    connect(this->m_handler, &QSocketNotifier::activated, this, &SignalHandler::handleSignal);
}

SignalHandler::~SignalHandler()
{
    for (auto signo : this->m_signos)
    {
        signal(signo, nullptr);
    }
    close(signalFD[0]);
    close(signalFD[1]);
}

SignalHandler* SignalHandler::m_instance = nullptr;
SignalHandler* SignalHandler::getDefault()
{
    if (!m_instance)
    {
        m_instance = new SignalHandler();
    }
    return m_instance;
}

void SignalHandler::addSignal(int signalToTrack)
{
    this->m_signos.insert(signalToTrack);
    signal(signalToTrack, signalHandler);
}

void SignalHandler::signalHandler(int signal)
{
    if (::write(signalFD[0], &signal, sizeof(signal)) < 0)
    {
        KLOG_WARNING() << "Couldn't write to file description" << signalFD[0] << "which errno is" << errno;
    }
}

void SignalHandler::handleSignal()
{
    int signal = 0;

    this->m_handler->setEnabled(false);
    auto readn = ::read(signalFD[1], &signal, sizeof(signal));
    this->m_handler->setEnabled(true);

    if (readn != sizeof(signal))
    {
        KLOG_WARNING() << "the number of bytes read(" << readn << ") isn't equal to" << sizeof(signal);
        return;
    }

    Q_EMIT signalReceived(signal);
}

}  // namespace Kiran