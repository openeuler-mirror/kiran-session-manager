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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtkmm.h>
#include "src/app/app-manager.h"
#include "src/client/client-manager.h"
#include "src/inhibitor-manager.h"
#include "src/session-manager.h"
#include "src/ui/ui-manager.h"
#include "src/utils.h"
#include "src/xsmp-server.h"

using namespace Kiran::Daemon;

#define DBUS_LAUNCH_COMMAND "dbus-launch"

bool start_dbus_session(int argc, char **argv)
{
    int i = 0;

    RETURN_VAL_IF_TRUE(!Glib::getenv("DBUS_SESSION_BUS_ADDRESS").empty(), true);
    RETURN_VAL_IF_TRUE(Kiran::StrUtils::startswith(argv[0], DBUS_LAUNCH_COMMAND), true);

    char **new_argv = (char **)g_malloc(argc + 3 * sizeof(*argv));

    new_argv[0] = (char *)DBUS_LAUNCH_COMMAND;
    new_argv[1] = (char *)"--exit-with-session";

    for (i = 0; i < argc; i++)
    {
        new_argv[i + 2] = argv[i];
    }

    new_argv[i + 2] = NULL;

    KLOG_DEBUG("Start session manager by dbus-launch.");

    if (!execvp(DBUS_LAUNCH_COMMAND, new_argv))
    {
        KLOG_WARNING("No session bus and could not exec dbus-launch: %s", g_strerror(errno));
        return false;
    }
    return true;
}

void init_env()
{
    using param_type = std::map<Glib::ustring, Glib::ustring>;
    param_type envs;

    try
    {
        auto key_regex = Glib::Regex::create("^[A-Za-z_][A-Za-z0-9_]*$", Glib::RegexCompileFlags::REGEX_OPTIMIZE);
        auto value_regex = Glib::Regex::create("^([[:blank:]]|[^[:cntrl:]])*$", Glib::RegexCompileFlags::REGEX_OPTIMIZE);

        for (auto key : Glib::listenv())
        {
            auto value = Glib::getenv(key);
            if (key_regex->match(value) && value_regex->match(value))
            {
                envs.emplace(key, value);
                KLOG_DEBUG("key: %s, value: %s.", key.c_str(), value.c_str());
            }
        }
        Utils::setenvs(envs);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return;
    }

    if (Glib::getenv("DISPLAY").empty())
    {
        auto display_name = Gdk::Display::get_default()->get_name();
        Utils::setenv("DISPLAY", display_name);
    }

#ifdef SUPPORT_CAJA
    // FIXME: 如果该变量不是MATE, 会导致caja无法启动,因此这里暂时设置为MATE,等更换文件管理器后改为KIRAN
    Utils::setenv("XDG_CURRENT_DESKTOP", "MATE");
#else
    if (Glib::getenv("XDG_CURRENT_DESKTOP").empty())
    {
        Utils::setenv("XDG_CURRENT_DESKTOP", "KIRAN");
    }
#endif
}

int main(int argc, char *argv[])
{
    klog_gtk3_init(std::string(), "kylinsec-session", PROJECT_NAME, PROJECT_NAME);

    // 这里做一个兼容性处理，当会话dbus-daemon没有启动时（一般不会出现），由会话管理进行拉起
    start_dbus_session(argc, argv);

    setlocale(LC_ALL, "");
    bindtextdomain(PROJECT_NAME, KSM_LOCALEDIR);
    bind_textdomain_codeset(PROJECT_NAME, "UTF-8");
    textdomain(PROJECT_NAME);

    gtk_init(&argc, &argv);
    Gtk::Main::init_gtkmm_internals();

    Glib::OptionContext context;
    Glib::OptionGroup group(PROJECT_NAME, "option group");

    // version
    Glib::OptionEntry version_entry;
    version_entry.set_long_name("version");
    version_entry.set_flags(Glib::OptionEntry::FLAG_NO_ARG);
    version_entry.set_description(N_("Output version infomation and exit"));

    group.add_entry(version_entry, [](const Glib::ustring &option_name, const Glib::ustring &value, bool has_value) -> bool {
        fmt::print("{0}: {1}\n", PROJECT_NAME, PROJECT_VERSION);
        return true;
    });

    group.set_translation_domain(PROJECT_NAME);
    context.set_main_group(group);

    try
    {
        context.parse(argc, argv);
    }
    catch (const Glib::Exception &e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return EXIT_FAILURE;
    }

    init_env();

    UIManager::global_init();
    AppManager::global_init();
    XsmpServer::global_init();
    InhibitorManager::global_init();
    ClientManager::global_init(XsmpServer::get_instance());
    SessionManager::global_init(AppManager::get_instance(),
                                ClientManager::get_instance(),
                                InhibitorManager::get_instance());
    SessionManager::get_instance()->start();

    gtk_main();

    return 0;
}
