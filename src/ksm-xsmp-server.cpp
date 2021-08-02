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

#include "src/ksm-xsmp-server.h"
#include "src/ksm-utils.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

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

struct KSMConnectionWatch
{
    uint32_t watch_id;
    sigc::connection protocol_timeout;
};

KSMXsmpServer::KSMXsmpServer() : num_listen_sockets_(0),
                                 num_local_listen_sockets_(0),
                                 listen_sockets_(NULL)
{
}

KSMXsmpServer::~KSMXsmpServer()
{
}

KSMXsmpServer *KSMXsmpServer::instance_ = nullptr;
void KSMXsmpServer::global_init()
{
    instance_ = new KSMXsmpServer();
    instance_->init();
}

void KSMXsmpServer::init()
{
    char error_string[BUFSIZ];
    SmsSetErrorHandler(&KSMXsmpServer::on_sms_error_handler_cb);
    IceSetIOErrorHandler(&KSMXsmpServer::on_ice_io_error_handler_cb);

    // 服务器初始化，这里会调用IceRegisterForProtocolReply函数
    if (!SmsInitialize(PROJECT_NAME,
                       PROJECT_VERSION,
                       KSMXsmpServer::on_new_client_connection_cb,
                       this,
                       NULL,
                       BUFSIZ,
                       error_string))
    {
        KLOG_WARNING("Init libSM failed: %s.", error_string);
        return;
    }

    this->listen_socket();
}

void KSMXsmpServer::listen_socket()
{
    char error_string[BUFSIZ];

    /* Create the XSMP socket. Older versions of IceListenForConnections
     * have a bug which causes the umask to be set to 0 on certain types
     * of failures. Probably not an issue on any modern systems, but
     * we'll play it safe.
    */

    auto saved_umask = umask(0);
    umask(saved_umask);
    // 监听网络连接，该函数会对每种socket类型(例如：unix/tcp/DECnet)返回一个地址
    if (!IceListenForConnections(&this->num_listen_sockets_,
                                 &this->listen_sockets_,
                                 BUFSIZ,
                                 error_string))
    {
        KLOG_WARNING("Listen connections failed: %s.", error_string);
        return;
    }
    umask(saved_umask);

    // 仅支持本机客户端连接
    for (int i = 0; i < this->num_listen_sockets_; i++)
    {
        auto socket_addr = IceGetListenConnectionString(this->listen_sockets_[i]);
        CONTINUE_IF_FALSE(socket_addr);

        if (Glib::str_has_prefix(socket_addr, "local/") ||
            Glib::str_has_prefix(socket_addr, "unix/"))
        {
            std::swap(this->listen_sockets_[i], this->listen_sockets_[this->num_local_listen_sockets_]);
            ++this->num_local_listen_sockets_;
        }
        free(socket_addr);
    }

    if (this->num_local_listen_sockets_ <= 0)
    {
        KLOG_WARNING("Not exists local listen socket.");
        return;
    }

    // 更新认证文件
    this->update_ice_authority();

    // 如果客户端未设置socket地址，则会从SESSION_MANAGER环境变量中读取。所以这里将服务器监听的地址写入该环境变量中
    auto network_ids = IceComposeNetworkIdList(this->num_local_listen_sockets_,
                                               this->listen_sockets_);
    KSMUtils::setenv("SESSION_MANAGER", network_ids);
    free(network_ids);

    for (uint32_t i = 0; i < this->num_local_listen_sockets_; i++)
    {
        auto channel = g_io_channel_unix_new(IceGetListenConnectionNumber(this->listen_sockets_[i]));
        g_io_add_watch(channel,
                       GIOCondition(G_IO_IN | G_IO_HUP | G_IO_ERR),
                       &KSMXsmpServer::on_accept_ice_connection_cb,
                       this->listen_sockets_[i]);
        g_io_channel_unref(channel);
    }
}

void KSMXsmpServer::update_ice_authority()
{
    std::vector<IceAuthFileEntry *> auth_entries;

    auto filename = IceAuthFileName();
    if (IceLockAuthFile(filename,
                        KSM_ICE_AUTH_RETRIES,
                        KSM_ICE_AUTH_INTERVAL,
                        KSM_ICE_AUTH_LOCK_TIMEOUT) != IceAuthLockSuccess)
    {
        KLOG_WARNING("Failed to lock file: %s.", filename);
        return;
    }

    SCOPE_EXIT({
        IceUnlockAuthFile(filename);
    });

    std::vector<std::string> network_ids;

    for (int32_t i = 0; i < this->num_local_listen_sockets_; i++)
    {
        auto network_id = IceGetListenConnectionString(this->listen_sockets_[i]);
        network_ids.push_back(network_id);
        free(network_id);
    }

    // 删除认证文件中包含network_ids的认证信息，然后重新设置新的认证信息
    auto fp = fopen(filename, "r+");
    SCOPE_EXIT({
        if (fp)
        {
            fclose(fp);
        }
    });

    if (fp)
    {
        IceAuthFileEntry *auth_entry = NULL;
        while ((auth_entry = IceReadAuthFileEntry(fp)) != NULL)
        {
            if (!auth_entry->network_id)
            {
                IceFreeAuthFileEntry(auth_entry);
                continue;
            }

            if (std::find(network_ids.begin(), network_ids.end(), std::string(auth_entry->network_id)) != network_ids.end())
            {
                IceFreeAuthFileEntry(auth_entry);
                continue;
            }

            auth_entries.push_back(auth_entry);
        }
        rewind(fp);
    }
    else
    {
        auto fd = open(filename, O_CREAT | O_WRONLY, 0600);
        fp = fdopen(fd, "w");
        if (!fp)
        {
            KLOG_WARNING("Failed to create file: %s", filename);
            return;
        }
    }

    for (int32_t i = 0; i < this->num_local_listen_sockets_; i++)
    {
        auth_entries.push_back(this->create_and_store_auth_entry("ICE", network_ids[i]));
        auth_entries.push_back(this->create_and_store_auth_entry("XSMP", network_ids[i]));
    }

    for (auto auth_entry : auth_entries)
    {
        IceWriteAuthFileEntry(fp, auth_entry);
        IceFreeAuthFileEntry(auth_entry);
    }
}

IceAuthFileEntry *KSMXsmpServer::create_and_store_auth_entry(const std::string &protocol_name,
                                                             const std::string &network_id)
{
    IceAuthFileEntry *file_entry = (IceAuthFileEntry *)malloc(sizeof(IceAuthFileEntry));
    file_entry->protocol_name = strdup(protocol_name.c_str());
    file_entry->protocol_data = NULL;
    file_entry->protocol_data_length = 0;
    file_entry->network_id = strdup(network_id.c_str());
    file_entry->auth_name = strdup(KSM_ICE_MAGIC_COOKIE_AUTH_NAME);
    file_entry->auth_data = IceGenerateMagicCookie(KSM_ICE_MAGIC_COOKIE_LEN);
    file_entry->auth_data_length = KSM_ICE_MAGIC_COOKIE_LEN;

    IceAuthDataEntry auth_data_entry;
    auth_data_entry.protocol_name = file_entry->protocol_name;
    auth_data_entry.network_id = file_entry->network_id;
    auth_data_entry.auth_name = file_entry->auth_name;
    auth_data_entry.auth_data = file_entry->auth_data;
    auth_data_entry.auth_data_length = file_entry->auth_data_length;
    IceSetPaAuthData(1, &auth_data_entry);

    return file_entry;
}

void KSMXsmpServer::on_sms_error_handler_cb(SmsConn sms_conn,
                                            Bool swap,
                                            int offending_minor_opcode,
                                            unsigned long offending_sequence_num,
                                            int error_class,
                                            int severity,
                                            IcePointer values)
{
    // 将错误信息重定向输出
    KLOG_WARNING("swap: %d, offending_minor_opcode: %d, offending_sequence_num: %ld, error_class: %d, severity: %d.",
                 swap,
                 offending_minor_opcode,
                 offending_sequence_num,
                 error_class,
                 severity);

    RETURN_IF_TRUE(severity == IceCanContinue);

    // 协议错误，让客户端关闭连接
    KLOG_WARNING("Send die message to client.");
    SmsDie(sms_conn);
}

void KSMXsmpServer::on_ice_io_error_handler_cb(IceConn ice_connection)
{
    KLOG_PROFILE("");
    /* 这里不错任何处理，仅仅打印日志方便定位，当出现该IO错误时，
     在下一次调用IceProcessMessages函数时会收到IceProcessMessagesIOError错误 */
}

Status KSMXsmpServer::on_new_client_connection_cb(SmsConn sms_conn,
                                                  SmPointer manager_data,
                                                  unsigned long *mask_ret,
                                                  SmsCallbacks *callbacks_ret,
                                                  char **failure_reason_ret)
{
    KSMXsmpServer *server = (KSMXsmpServer *)(manager_data);
    auto ice_conn = SmsGetIceConnection(sms_conn);

    KLOG_DEBUG("New client connection: %p.", ice_conn);

    // 如果连接成功，则取消超时检测
    auto watch = (KSMConnectionWatch *)(ice_conn->context);
    watch->protocol_timeout.disconnect();

    server->new_client_connected_.emit(mask_ret, callbacks_ret);
    return True;
}

gboolean KSMXsmpServer::on_accept_ice_connection_cb(GIOChannel *source,
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
    auto watch = new KSMConnectionWatch();
    ice_conn->context = watch;

    auto timeout = Glib::MainContext::get_default()->signal_timeout();
    watch->protocol_timeout = timeout.connect_seconds(sigc::bind(&KSMXsmpServer::on_ice_protocol_timeout_cb, ice_conn), 5);
    watch->watch_id = g_io_add_watch(channel,
                                     GIOCondition(G_IO_IN | G_IO_ERR),
                                     &KSMXsmpServer::on_auth_iochannel_watch_cb,
                                     ice_conn);
    g_io_channel_unref(channel);

    return TRUE;
}

bool KSMXsmpServer::on_ice_protocol_timeout_cb(IceConn ice_conn)
{
    KLOG_PROFILE("");

    auto watch = (KSMConnectionWatch *)(ice_conn->context);
    KSMXsmpServer::free_connection_watch(watch);
    KSMXsmpServer::disconnect_ice_connection(ice_conn);
    return false;
}

gboolean KSMXsmpServer::on_auth_iochannel_watch_cb(GIOChannel *source,
                                                   GIOCondition condition,
                                                   gpointer user_data)
{
    KLOG_PROFILE("");

    auto ice_conn = (IceConn)(user_data);
    auto watch = (KSMConnectionWatch *)(ice_conn->context);

    auto status = IceProcessMessages(ice_conn, NULL, NULL);
    RETURN_VAL_IF_TRUE(status == IceProcessMessagesSuccess, TRUE);

    switch (status)
    {
    case IceProcessMessagesIOError:
        KSMXsmpServer::free_connection_watch(watch);
        KSMXsmpServer::disconnect_ice_connection(ice_conn);
        break;
    case IceProcessMessagesConnectionClosed:
        KSMXsmpServer::free_connection_watch(watch);
        break;
    default:
        break;
    }

    KSMXsmpServer::get_instance()->ice_conn_status_changed_.emit(status, ice_conn);
    return FALSE;
}

void KSMXsmpServer::disconnect_ice_connection(IceConn ice_conn)
{
    IceSetShutdownNegotiation(ice_conn, FALSE);
    IceCloseConnection(ice_conn);
}

void KSMXsmpServer::free_connection_watch(KSMConnectionWatch *watch)
{
    if (watch->watch_id)
    {
        g_source_remove(watch->watch_id);
        watch->watch_id = 0;
    }

    if (watch->protocol_timeout)
    {
        watch->protocol_timeout.disconnect();
    }

    delete watch;
}

}  // namespace Kiran
