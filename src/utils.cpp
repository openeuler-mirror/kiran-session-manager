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

#include "src/utils.h"
#include <sys/time.h>
#include "lib/base.h"

namespace Kiran
{
namespace Daemon
{
Glib::Rand Utils::rand_;

/* The XSMP spec defines the ID as:
*
* Version: "1"
* Address type and address:
*   "1" + an IPv4 address as 8 hex digits
*   "2" + a DECNET address as 12 hex digits
*   "6" + an IPv6 address as 32 hex digits
* Time stamp: milliseconds since UNIX epoch as 13 decimal digits
* Process-ID type and process-ID:
*   "1" + POSIX PID as 10 decimal digits
* Sequence number as 4 decimal digits
*
* XSMP client IDs are supposed to be globally unique: if
* SmsGenerateClientID() is unable to determine a network
* address for the machine, it gives up and returns %NULL.
* MATE and KDE have traditionally used a fourth address
* format in this case:
*   "0" + 16 random hex digits
*
* We don't even bother trying SmsGenerateClientID(), since the
* user's IP address is probably "192.168.1.*" anyway, so a random
* number is actually more likely to be globally unique.
*/

std::string Utils::generate_startup_id()
{
    static int sequence = -1;
    static uint32_t rand1 = 0;
    static uint32_t rand2 = 0;
    static pid_t pid = 0;
    struct timeval tv;

    if (!rand1)
    {
        rand1 = Utils::rand_.get_int();
        rand2 = Utils::rand_.get_int();
        pid = getpid();
    }

    sequence = (sequence + 1) % 10000;
    gettimeofday(&tv, NULL);
    g_autofree char *ret = g_strdup_printf("10%.04x%.04x%.10lu%.3u%.10lu%.4d",
                                           rand1,
                                           rand2,
                                           (unsigned long)tv.tv_sec,
                                           (unsigned)tv.tv_usec,
                                           (unsigned long)pid,
                                           sequence);
    return std::string(ret);
}

std::vector<std::string> Utils::get_autostart_dirs()
{
    std::vector<std::string> dirs;

    dirs.push_back(Glib::build_filename(Glib::get_user_config_dir(), "autostart"));

    for (auto system_config_dir : Glib::get_system_config_dirs())
    {
        dirs.push_back(Glib::build_filename(system_config_dir, "autostart"));
    }

    return dirs;
}

void Utils::setenv(const std::string &key, const std::string &value)
{
    KLOG_DEBUG("key: %s, value: %s.", key.c_str(), value.c_str());

    Glib::setenv(key, value);
    Utils::setenv_to_dbus(key, value);
    Utils::setenv_to_systemd(key, value);
}

void Utils::setenvs(const std::map<Glib::ustring, Glib::ustring> &envs)
{
    try
    {
        auto daemon_proxy = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BUS_TYPE_SESSION,
                                                                  DAEMON_DBUS_NAME,
                                                                  DAEMON_DBUS_OBJECT_PATH,
                                                                  DAEMON_DBUS_INTERFACE_NAME);
        using ParamType = std::tuple<std::map<Glib::ustring, Glib::ustring>>;
        auto parameter = Glib::Variant<ParamType>::create(std::make_tuple(envs));
        daemon_proxy->call_sync("UpdateActivationEnvironment", parameter);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return;
    }

    try
    {
        using ParamType = std::tuple<std::vector<Glib::ustring>>;
        std::vector<Glib::ustring> list_env;

        for (auto iter : envs)
        {
            auto env = fmt::format("{0}={1}", iter.first, iter.second);
            list_env.push_back(env);
        }

        KLOG_DEBUG("Set environments: %s.", StrUtils::join(list_env, ";").c_str());
        auto systemd_proxy = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BUS_TYPE_SESSION,
                                                                   SYSTEMD_DBUS_NAME,
                                                                   SYSTEMD_DBUS_OBJECT_PATH,
                                                                   SYSTEMD_DBUS_INTERFACE_NAME);
        auto parameter = Glib::Variant<ParamType>::create(std::make_tuple(list_env));
        systemd_proxy->call_sync("SetEnvironment", parameter);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return;
    }
}

uint32_t Utils::generate_cookie()
{
    return Utils::rand_.get_int_range(1, G_MAXINT32);
}

KSMPhase Utils::phase_str2enum(const std::string &phase_str)
{
    switch (shash(phase_str.c_str()))
    {
    case "Initialization"_hash:
        return KSMPhase::KSM_PHASE_INITIALIZATION;
    case "WindowManager"_hash:
        return KSMPhase::KSM_PHASE_WINDOW_MANAGER;
    case "Panel"_hash:
        return KSMPhase::KSM_PHASE_PANEL;
    case "Desktop"_hash:
        return KSMPhase::KSM_PHASE_DESKTOP;
    default:
        return KSMPhase::KSM_PHASE_APPLICATION;
    }
    return KSMPhase::KSM_PHASE_APPLICATION;
}

std::string Utils::phase_enum2str(KSMPhase phase)
{
    switch (phase)
    {
    case KSMPhase::KSM_PHASE_IDLE:
        return "Idle";
    case KSMPhase::KSM_PHASE_INITIALIZATION:
        return "Initialization";
    case KSMPhase::KSM_PHASE_WINDOW_MANAGER:
        return "WindowManager";
    case KSMPhase::KSM_PHASE_PANEL:
        return "Panel";
    case KSMPhase::KSM_PHASE_DESKTOP:
        return "Desktop";
    case KSMPhase::KSM_PHASE_APPLICATION:
        return "Application";
    case KSMPhase::KSM_PHASE_RUNNING:
        return "Running";
    case KSMPhase::KSM_PHASE_QUERY_END_SESSION:
        return "QueryEndSession";
    case KSMPhase::KSM_PHASE_END_SESSION_PHASE1:
        return "EndSessionPhase1";
    case KSMPhase::KSM_PHASE_END_SESSION_PHASE2:
        return "EndSessionPhase2";
    case KSMPhase::KSM_PHASE_EXIT:
        return "Exit";
    default:
        KLOG_WARNING("The phase %d is invalid.", phase);
        return std::string();
    }
    return std::string();
}

void Utils::setenv_to_dbus(const std::string &key, const std::string &value)
{
    try
    {
        auto daemon_proxy = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BUS_TYPE_SESSION,
                                                                  DAEMON_DBUS_NAME,
                                                                  DAEMON_DBUS_OBJECT_PATH,
                                                                  DAEMON_DBUS_INTERFACE_NAME);
        using param_type = std::tuple<std::map<Glib::ustring, Glib::ustring>>;

        std::map<Glib::ustring, Glib::ustring> envs{{key, value}};
        auto parameter = Glib::Variant<param_type>::create(std::make_tuple(envs));
        daemon_proxy->call_sync("UpdateActivationEnvironment", parameter);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return;
    }
}

void Utils::setenv_to_systemd(const std::string &key, const std::string &value)
{
    try
    {
        auto systemd_proxy = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BUS_TYPE_SESSION,
                                                                   SYSTEMD_DBUS_NAME,
                                                                   SYSTEMD_DBUS_OBJECT_PATH,
                                                                   SYSTEMD_DBUS_INTERFACE_NAME);
        using param_type = std::tuple<std::vector<Glib::ustring>>;
        auto env = fmt::format("{0}={1}", key, value);
        auto parameter = Glib::Variant<param_type>::create(std::make_tuple(std::vector<Glib::ustring>{env}));
        systemd_proxy->call_sync("SetEnvironment", parameter);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return;
    }
}
}  // namespace Daemon
}  // namespace Kiran