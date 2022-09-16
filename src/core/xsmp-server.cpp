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

#include "src/core/xsmp-server.h"
#include "lib/base/base.h"

#include <X11/ICE/ICEconn.h>
#include <X11/ICE/ICElib.h>
#include <X11/ICE/ICEutil.h>
#include <X11/SM/SMlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <QTimer>
#include "src/core/utils.h"

extern "C"
{
#define ICE_t
#define TRANS_SERVER
#include <X11/Xtrans/Xtrans.h>
#undef ICE_t
#undef TRANS_SERVER
}

namespace Kiran
{
#define KSM_ICE_MAGIC_COOKIE_AUTH_NAME "MIT-MAGIC-COOKIE-1"
#define KSM_ICE_MAGIC_COOKIE_LEN 16

// 重试次数
#define KSM_ICE_AUTH_RETRIES 6
// 重试前等待的时间（秒）
#define KSM_ICE_AUTH_INTERVAL 4
// 如果锁已经存在，则超过该时间（秒）后之前的锁强制释放，如果设置为0则直接解除之前的锁
#define KSM_ICE_AUTH_LOCK_TIMEOUT 16

struct ConnectionWatch
{
    uint32_t watchID;
    QTimer protocolTimeout;
    QMetaObject::Connection connection;
};

static void onSmsErrorHandler(SmsConn smsConn,
                              Bool swap,
                              int offendingMinorOpcode,
                              unsigned long offendingSequenceNum,
                              int errorClass,
                              int severity,
                              IcePointer values)
{
    // 将错误信息重定向输出
    KLOG_WARNING() << "Swap: " << swap << ", offending_minor_opcode: " << offendingMinorOpcode
                   << ", offending_sequence_num: " << offendingSequenceNum
                   << ", error_class: " << errorClass << ", severity: " << severity;

    RETURN_IF_TRUE(severity == IceCanContinue);

    // 协议错误，让客户端关闭连接
    KLOG_WARNING("Send die message to client.");
    SmsDie(smsConn);
}

static void onIceIoErrorHandler(IceConn iceConnection)
{
    KLOG_WARNING() << "Receive io error message. connection: " << iceConnection;
    /* 这里不错任何处理，仅仅打印日志方便定位，当出现该IO错误时，
     在下一次调用IceProcessMessages函数时会收到IceProcessMessagesIOError错误 */
}

static Status onNewClientConnection(SmsConn smsConn,
                                    SmPointer managerData,
                                    unsigned long *maskRet,
                                    SmsCallbacks *callbacksRet,
                                    char **failureReasonRet)
{
    XsmpServer *server = (XsmpServer *)(managerData);
    auto iceConn = SmsGetIceConnection(smsConn);

    KLOG_DEBUG("New client connection: %p.", iceConn);

    // 如果连接成功，则取消超时检测
    auto watch = (ConnectionWatch *)(iceConn->context);
    watch->protocolTimeout.stop();

    Q_EMIT server->newClientConnected(maskRet, callbacksRet);
    return True;
}

static void freeConnectionWatch(ConnectionWatch *watch)
{
    if (watch->watchID)
    {
        g_source_remove(watch->watchID);
        watch->watchID = 0;
    }

    watch->protocolTimeout.stop();
    QObject::disconnect(watch->connection);

    delete watch;
}

static void disconnectIceConnection(IceConn ice_conn)
{
    IceSetShutdownNegotiation(ice_conn, FALSE);
    IceCloseConnection(ice_conn);
}

static bool onIceProtocolTimeout(IceConn ice_conn)
{
    KLOG_DEBUG("Ice protocol timeout.");

    auto watch = (ConnectionWatch *)(ice_conn->context);
    freeConnectionWatch(watch);
    disconnectIceConnection(ice_conn);
    return false;
}

XsmpServer::XsmpServer() : QObject(nullptr),
                           m_numListenSockets(0),
                           m_numLocalListenSockets(0),
                           m_listenSockets(NULL)
{
}

XsmpServer::~XsmpServer()
{
}

XsmpServer *XsmpServer::m_instance = nullptr;
void XsmpServer::globalInit()
{
    m_instance = new XsmpServer();
    m_instance->init();
}

void XsmpServer::init()
{
    KLOG_DEBUG() << "Xsmp server init.";

    char error_string[BUFSIZ];
    SmsSetErrorHandler(onSmsErrorHandler);
    IceSetIOErrorHandler(onIceIoErrorHandler);

    // 服务器初始化，这里会调用IceRegisterForProtocolReply函数
    if (!SmsInitialize(PROJECT_NAME,
                       PROJECT_VERSION,
                       onNewClientConnection,
                       this,
                       NULL,
                       BUFSIZ,
                       error_string))
    {
        KLOG_WARNING("Init libSM failed: %s.", error_string);
        return;
    }

    this->listenSocket();
}

void XsmpServer::listenSocket()
{
    char errorString[BUFSIZ];

    _IceTransNoListen("tcp");

    /* Create the XSMP socket. Older versions of IceListenForConnections
     * have a bug which causes the umask to be set to 0 on certain types
     * of failures. Probably not an issue on any modern systems, but
     * we'll play it safe.
    */
    auto savedUmask = umask(0);
    umask(savedUmask);
    // 监听网络连接，该函数会对每种socket类型(例如：unix/tcp/DECnet)返回一个地址
    if (!IceListenForConnections(&this->m_numListenSockets,
                                 &this->m_listenSockets,
                                 BUFSIZ,
                                 errorString))
    {
        KLOG_WARNING("Listen connections failed: %s.", errorString);
        return;
    }
    umask(savedUmask);

    // 仅支持本机客户端连接
    for (int i = 0; i < this->m_numListenSockets; i++)
    {
        auto pSocketAddr = IceGetListenConnectionString(this->m_listenSockets[i]);
        CONTINUE_IF_FALSE(pSocketAddr);

        auto socketAddr = QString(pSocketAddr);

        if (socketAddr.startsWith("local/") || socketAddr.startsWith("unix/"))
        {
            std::swap(this->m_listenSockets[i], this->m_listenSockets[this->m_numLocalListenSockets]);
            ++this->m_numLocalListenSockets;
        }

        free(pSocketAddr);
    }

    if (this->m_numLocalListenSockets <= 0)
    {
        KLOG_WARNING("Not exists local listen socket.");
        return;
    }

    // 更新认证文件
    this->updateIceAuthority();

    // 如果客户端未设置socket地址，则会从SESSION_MANAGER环境变量中读取。所以这里将服务器监听的地址写入该环境变量中
    auto networkIds = IceComposeNetworkIdList(this->m_numLocalListenSockets,
                                              this->m_listenSockets);
    Utils::getDefault()->setEnv("SESSION_MANAGER", networkIds);
    free(networkIds);

    for (int32_t i = 0; i < this->m_numLocalListenSockets; i++)
    {
        auto channel = g_io_channel_unix_new(IceGetListenConnectionNumber(this->m_listenSockets[i]));
        g_io_add_watch(channel,
                       GIOCondition(G_IO_IN | G_IO_HUP | G_IO_ERR),
                       &XsmpServer::onAcceptIceConnection,
                       this->m_listenSockets[i]);
        g_io_channel_unref(channel);
    }
}

void XsmpServer::updateIceAuthority()
{
    std::vector<IceAuthFileEntry *> authEntries;

    auto fileName = IceAuthFileName();
    if (IceLockAuthFile(fileName,
                        KSM_ICE_AUTH_RETRIES,
                        KSM_ICE_AUTH_INTERVAL,
                        KSM_ICE_AUTH_LOCK_TIMEOUT) != IceAuthLockSuccess)
    {
        KLOG_WARNING("Failed to lock file: %s.", fileName);
        return;
    }

    SCOPE_EXIT({
        IceUnlockAuthFile(fileName);
    });

    std::vector<std::string> networkIds;

    for (int32_t i = 0; i < this->m_numLocalListenSockets; i++)
    {
        auto networkID = IceGetListenConnectionString(this->m_listenSockets[i]);
        networkIds.push_back(networkID);
        free(networkID);
    }

    // 删除认证文件中包含network_ids的认证信息，然后重新设置新的认证信息
    auto fp = fopen(fileName, "r+");
    SCOPE_EXIT(
        {
            if (fp)
            {
                fclose(fp);
            }
        });

    if (fp)
    {
        IceAuthFileEntry *authEntry = NULL;
        while ((authEntry = IceReadAuthFileEntry(fp)) != NULL)
        {
            if (!authEntry->network_id)
            {
                IceFreeAuthFileEntry(authEntry);
                continue;
            }

            if (std::find(networkIds.begin(), networkIds.end(), std::string(authEntry->network_id)) != networkIds.end())
            {
                IceFreeAuthFileEntry(authEntry);
                continue;
            }

            authEntries.push_back(authEntry);
        }
        rewind(fp);
    }
    else
    {
        auto fd = open(fileName, O_CREAT | O_WRONLY, 0600);
        fp = fdopen(fd, "w");
        if (!fp)
        {
            KLOG_WARNING("Failed to create file: %s", fileName);
            return;
        }
    }

    for (int32_t i = 0; i < this->m_numLocalListenSockets; i++)
    {
        authEntries.push_back((IceAuthFileEntry *)this->createAndStoreAuthEntry("ICE", networkIds[i]));
        authEntries.push_back((IceAuthFileEntry *)this->createAndStoreAuthEntry("XSMP", networkIds[i]));
    }

    for (auto authEntry : authEntries)
    {
        IceWriteAuthFileEntry(fp, authEntry);
        IceFreeAuthFileEntry(authEntry);
    }
}

void *XsmpServer::createAndStoreAuthEntry(const std::string &protocolName,
                                          const std::string &networkID)
{
    IceAuthFileEntry *fileEntry = (IceAuthFileEntry *)malloc(sizeof(IceAuthFileEntry));
    fileEntry->protocol_name = strdup(protocolName.c_str());
    fileEntry->protocol_data = NULL;
    fileEntry->protocol_data_length = 0;
    fileEntry->network_id = strdup(networkID.c_str());
    fileEntry->auth_name = strdup(KSM_ICE_MAGIC_COOKIE_AUTH_NAME);
    fileEntry->auth_data = IceGenerateMagicCookie(KSM_ICE_MAGIC_COOKIE_LEN);
    fileEntry->auth_data_length = KSM_ICE_MAGIC_COOKIE_LEN;

    IceAuthDataEntry authDataEntry;
    authDataEntry.protocol_name = fileEntry->protocol_name;
    authDataEntry.network_id = fileEntry->network_id;
    authDataEntry.auth_name = fileEntry->auth_name;
    authDataEntry.auth_data = fileEntry->auth_data;
    authDataEntry.auth_data_length = fileEntry->auth_data_length;
    IceSetPaAuthData(1, &authDataEntry);

    return fileEntry;
}

gboolean XsmpServer::onAcceptIceConnection(GIOChannel *source,
                                           GIOCondition condition,
                                           gpointer data)
{
    IceAcceptStatus status;
    IceListenObj listener = (IceListenObj)data;

    auto ice_conn = IceAcceptConnection(listener, &status);
    if (status != IceAcceptSuccess)
    {
        KLOG_DEBUG("Accept ice connection with error status. status: %d", status);
        return TRUE;
    }

    auto fd = IceConnectionNumber(ice_conn);
    fcntl(fd, F_SETFD, fcntl(fd, F_GETFD, 0) | FD_CLOEXEC);

    auto channel = g_io_channel_unix_new(fd);
    auto watch = new ConnectionWatch();
    ice_conn->context = watch;

    watch->connection = QObject::connect(&watch->protocolTimeout, &QTimer::timeout, std::bind(&onIceProtocolTimeout, ice_conn));
    watch->protocolTimeout.start(5000);

    watch->watchID = g_io_add_watch(channel,
                                    GIOCondition(G_IO_IN | G_IO_ERR),
                                    &XsmpServer::onAuthIochannelWatch,
                                    ice_conn);
    g_io_channel_unref(channel);

    return TRUE;
}

gboolean XsmpServer::onAuthIochannelWatch(GIOChannel *source,
                                          GIOCondition condition,
                                          gpointer userData)
{
    auto iceConn = (IceConn)(userData);
    auto watch = (ConnectionWatch *)(iceConn->context);

    auto status = IceProcessMessages(iceConn, NULL, NULL);
    RETURN_VAL_IF_TRUE(status == IceProcessMessagesSuccess, TRUE);

    KLOG_DEBUG() << "Receive io event: " << condition << " status: " << status;

    switch (status)
    {
    case IceProcessMessagesIOError:
        freeConnectionWatch(watch);
        disconnectIceConnection(iceConn);
        break;
    case IceProcessMessagesConnectionClosed:
        freeConnectionWatch(watch);
        break;
    default:
        break;
    }

    Q_EMIT XsmpServer::getInstance()->iceConnStatusChanged(status, iceConn);

    return FALSE;
}

}  // namespace Kiran
