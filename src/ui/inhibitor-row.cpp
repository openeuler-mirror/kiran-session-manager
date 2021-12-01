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

#include "src/ui/inhibitor-row.h"
#include "lib/base.h"
#include "src/inhibitor-manager.h"

namespace Kiran
{
namespace Daemon
{
InhibitorRow::InhibitorRow(GtkBox *box,
                           const Glib::RefPtr<Gtk::Builder> &builder,
                           std::shared_ptr<Inhibitor> inhibitor) : Gtk::Box(box),
                                                                   builder_(builder),
                                                                   inhibitor_(inhibitor)
{
    KLOG_DEBUG("app_id : %s.", inhibitor->app_id.c_str());
    this->init();
}

InhibitorRow *InhibitorRow::create(std::shared_ptr<Inhibitor> inhibitor)
{
    InhibitorRow *row = NULL;
    try
    {
        auto builder = Gtk::Builder::create_from_resource(GRESOURCE_PATH "/ui/inhibitor-row");
        builder->get_widget_derived<InhibitorRow>("row-box", row, inhibitor);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("%s", e.what().c_str());
    }
    return row;
}

void InhibitorRow::init()
{
    Gtk::Image *app_icon = NULL;
    Gtk::Label *app_name = NULL;
    Gtk::Label *description = NULL;

    auto app_id = StrUtils::trim(this->inhibitor_->app_id);

    if (!StrUtils::endswith(app_id, ".desktop"))
    {
        app_id += ".desktop";
    }

    // path_is_absolute函数只在window平台有效
    if (Glib::path_is_absolute(this->inhibitor_->app_id) || app_id[0] == '/')
    {
        this->app_info_ = Gio::DesktopAppInfo::create_from_filename(this->inhibitor_->app_id);
    }
    else
    {
        this->app_info_ = Gio::DesktopAppInfo::create(this->inhibitor_->app_id);
    }

    try
    {
        this->builder_->get_widget<Gtk::Image>("app_icon", app_icon);
        this->builder_->get_widget<Gtk::Label>("app_name", app_name);
        this->builder_->get_widget<Gtk::Label>("description", description);

        if (this->app_info_)
        {
            app_icon->set(Glib::RefPtr<const Gio::Icon>::cast_dynamic(this->app_info_->get_icon()), Gtk::IconSize(Gtk::ICON_SIZE_DIALOG));
            app_name->set_label(this->app_info_->get_locale_string("Name"));
        }
        else
        {
            app_icon->set(Gdk::Pixbuf::create_from_resource(GRESOURCE_PATH "/image/app-missing", 48, 48));
            app_name->set_label(this->inhibitor_->app_id);
        }
        description->set_label(this->inhibitor_->reason);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("%s", e.what().c_str());
    }
}

}  // namespace Daemon

}  // namespace Kiran
