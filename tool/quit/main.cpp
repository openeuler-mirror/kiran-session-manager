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
 * Author:     yangfeng <yangfeng@kylinsec.com.cn>
 */

#include <giomm.h>
#include <glib/gi18n.h>
#include <gtk3-log-i.h>
#include <locale.h>
#include "config.h"
#include "ksm-i.h"

int main(int argc, char *argv[])
{
    Gio::init();

    // 设置locale以便正确处理国际化字符串
    setlocale(LC_ALL, "");
    bindtextdomain(PROJECT_NAME, KSM_LOCALEDIR);
    bind_textdomain_codeset(PROJECT_NAME, "UTF-8");
    textdomain(PROJECT_NAME);

    // 命令行选项
    gboolean logout = FALSE;
    gboolean power_off = FALSE;
    gboolean reboot = FALSE;

    // 解析命令行参数
    GOptionEntry entries[] = {
        {"logout", 'l', 0, G_OPTION_ARG_NONE, &logout, _("Log out"), NULL},
        {"power-off", 'p', 0, G_OPTION_ARG_NONE, &power_off, _("Power off"), NULL},
        {"reboot", 'r', 0, G_OPTION_ARG_NONE, &reboot, _("Reboot"), NULL},
        {NULL}};

    GError *error = NULL;
    GOptionContext *context = g_option_context_new("");
    g_option_context_add_main_entries(context, entries, NULL);
    if (!g_option_context_parse(context, &argc, &argv, &error))
    {
        g_printerr("option parsing failed: %s\n", error->message);
        g_error_free(error);
        g_option_context_free(context);
        return 1;
    }
    g_option_context_free(context);

    Glib::RefPtr<Gio::DBus::Proxy> proxy;
    try
    {
        proxy = Gio::DBus::Proxy::create_for_bus_sync(Gio::DBus::BUS_TYPE_SESSION,
                                                      KSM_DBUS_NAME,
                                                      KSM_DBUS_OBJECT_PATH,
                                                      KSM_DBUS_INTERFACE_NAME);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("%s", e.what().c_str());
        return -1;
    }

    if (!proxy)
    {
        KLOG_WARNING("Cannot create dbus proxy for %s:%s.", KSM_DBUS_NAME, KSM_DBUS_OBJECT_PATH);
        return -1;
    }

    if (logout)
    {
        // 注销
        try
        {
            Glib::Variant<uint> variant = Glib::Variant<uint>::create(0);
            Glib::VariantContainerBase container = Glib::VariantContainerBase::create_tuple(variant);
            proxy->call_sync("Logout", container);
        }
        catch (const Glib::Error &e)
        {
            KLOG_WARNING("failed to call Logout: %s", e.what().c_str());
            return -1;
        }
    }
    else if (power_off)
    {
        // 关机
        try
        {
            proxy->call_sync("Shutdown");
        }
        catch (const Glib::Error &e)
        {
            KLOG_WARNING("failed to call Shutdown: %s", e.what().c_str());
            return -1;
        }
    }
    else if (reboot)
    {
        // 重启
        try
        {
            proxy->call_sync("Reboot");
        }
        catch (const Glib::Error &e)
        {
            KLOG_WARNING("failed to call Reboot: %s", e.what().c_str());
            return -1;
        }
    }
    else
    {
        g_print("No action specified.\n");
        return 1;
    }

    return 0;
}