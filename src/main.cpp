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

#include <gdkmm/wrap_init.h>
#include <glib/gi18n.h>
#include <gtkmm.h>
#include <gtkmm/wrap_init.h>
#include "src/client/ksm-client-manager.h"
#include "src/ksm-app-manager.h"
#include "src/ksm-manager.h"
#include "src/ksm-utils.h"
#include "src/ksm-xsmp-server.h"

void init_env()
{
    using param_type = std::map<Glib::ustring, Glib::ustring>;
    param_type envs;

    try
    {
        auto key_regex = Glib::Regex::create("^[A-Za-z_][A-Za-z0-9]*$", Glib::RegexCompileFlags::REGEX_OPTIMIZE);
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
        Kiran::KSMUtils::setenvs(envs);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return;
    }

    if (Glib::getenv("DISPLAY").empty())
    {
        auto display_name = Gdk::Display::get_default()->get_name();
        Kiran::KSMUtils::setenv("DISPLAY", display_name);
    }

    if (Glib::getenv("XDG_CURRENT_DESKTOP").empty())
    {
        Kiran::KSMUtils::setenv("XDG_CURRENT_DESKTOP", "KIRAN");
    }
}

int main(int argc, char *argv[])
{
    klog_gtk3_init(std::string(), "kylinsec-session", PROJECT_NAME, PROJECT_NAME);
    Gio::init();

    setlocale(LC_ALL, "");
    bindtextdomain(PROJECT_NAME, KSM_LOCALEDIR);
    bind_textdomain_codeset(PROJECT_NAME, "UTF-8");
    textdomain(PROJECT_NAME);

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

    gtk_init(NULL, NULL);
    Gdk::wrap_init();
    Gtk::wrap_init();

    init_env();

    Kiran::KSMAppManager::global_init();
    Kiran::KSMXsmpServer::global_init();
    Kiran::KSMClientManager::global_init(Kiran::KSMXsmpServer::get_instance());
    Kiran::KSMManager::global_init(Kiran::KSMAppManager::get_instance(),
                                   Kiran::KSMClientManager::get_instance());
    Kiran::KSMManager::get_instance()->start();

    gtk_main();

    return 0;
}
