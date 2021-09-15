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

#include "src/ui/exit-query-window.h"
#include <glib/gi18n.h>
#include "lib/base.h"
#include "src/inhibitor-manager.h"
#include "src/ui/inhibitor-row.h"

namespace Kiran
{
namespace Daemon
{
#define BLUR_APREC 16
#define BLUR_ZPREC 7
#define BLUR_RADIUS 10
#define BLUR_MAX_CHANNEL_NUM 4

ExitQueryWindow::ExitQueryWindow(GtkWindow *window,
                                 const Glib::RefPtr<Gtk::Builder> &builder,
                                 PowerAction power_action) : Gtk::Window(window),
                                                             builder_(builder),
                                                             power_action_(power_action),
                                                             blur_alpha_(0),
                                                             content_(NULL),
                                                             title_(NULL),
                                                             title_desc_(NULL),
                                                             ok_(NULL),
                                                             cancel_(NULL)
{
    this->blur_alpha_ = (int32_t)((1 << BLUR_APREC) * (1.0f - expf(-2.3f / (BLUR_RADIUS + 1.f))));

    try
    {
        this->builder_->get_widget<Gtk::Box>("content", this->content_);
        this->builder_->get_widget<Gtk::Label>("title", this->title_);
        this->builder_->get_widget<Gtk::Label>("title_desc", this->title_desc_);
        this->builder_->get_widget<Gtk::Box>("inhibitors", this->inhibitors_);
        this->builder_->get_widget<Gtk::Button>("ok", this->ok_);
        this->builder_->get_widget<Gtk::Button>("cancel", this->cancel_);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("%s.", e.what().c_str());
    }

    this->init();
}

std::shared_ptr<ExitQueryWindow> ExitQueryWindow::create(PowerAction power_action)
{
    ExitQueryWindow *window = NULL;
    try
    {
        auto builder = Gtk::Builder::create_from_resource(GRESOURCE_PATH "/ui/exit-query");
        builder->get_widget_derived<ExitQueryWindow>("window", window, power_action);
    }
    catch (const Glib::Error &e)
    {
        KLOG_WARNING("%s", e.what().c_str());
    }

    return std::shared_ptr<ExitQueryWindow>(window);
}

void ExitQueryWindow::on_realize()
{
    Gtk::Window::on_realize();
    // 不让窗口管理器接管相关事件处理，否则会导致该窗口在扩展屏时无法覆盖panel区域
    this->get_window()->set_override_redirect(true);
}

bool ExitQueryWindow::on_draw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    KLOG_PROFILE("");

    if (!this->background_surface_)
    {
        this->background_surface_ = this->get_blur_background_surface();
    }

    auto surface_pattern = Cairo::SurfacePattern::create(this->background_surface_);
    cr->save();
    cr->set_source(surface_pattern);
    cr->paint();
    cr->restore();

    this->propagate_draw(*this->get_child(), cr);
    return true;
}

void ExitQueryWindow::init()
{
    this->set_decorated(false);
    this->set_skip_taskbar_hint(true);
    this->set_skip_pager_hint(true);
    this->set_keep_above();

    auto inhibitors = InhibitorManager::get_instance()->get_inhibitors();

    this->title_->set_label(fmt::format(_("Closing {0} apps"), inhibitors.size()));
    this->title_desc_->set_label(_("If you want to go back and save your work, click 'cancel' and finish what you want to do"));

    for (auto inhibitor : inhibitors)
    {
        auto inhibitor_row = InhibitorRow::create(inhibitor);
        this->inhibitors_->pack_start(*inhibitor_row, Gtk::PackOptions::PACK_SHRINK);
    }

    switch (this->power_action_)
    {
    case PowerAction::POWER_ACTION_LOGOUT:
        this->ok_->set_label(_("Forced logout"));
        break;
    case PowerAction::POWER_ACTION_SHUTDOWN:
        this->ok_->set_label(_("Forced shutdown"));
        break;
    case PowerAction::POWER_ACTION_REBOOT:
        this->ok_->set_label(_("Forced reboot"));
        break;
    default:
        KLOG_WARNING("The power action is unsupported.");
        break;
    }

    this->on_monitor_changed();

    // 处理信号
    this->ok_->signal_clicked().connect([this]() {
        this->response_.emit(ExitQueryResponse::EXIT_QUERY_RESPONSE_OK);
    });
    this->cancel_->signal_clicked().connect([this]() {
        this->response_.emit(ExitQueryResponse::EXIT_QUERY_RESPONSE_CANCEL);
    });
    this->get_screen()->signal_monitors_changed().connect(sigc::mem_fun(this, &ExitQueryWindow::on_monitor_changed));
    this->get_screen()->signal_size_changed().connect(sigc::mem_fun(this, &ExitQueryWindow::on_monitor_changed));

    this->content_->signal_size_allocate().connect(sigc::mem_fun(this, &ExitQueryWindow::on_content_size_allocate_cb));
}

Cairo::RefPtr<Cairo::ImageSurface> ExitQueryWindow::get_blur_background_surface()
{
    // FIXME: 有一定几率取到的根窗口图片为空白，导致背景色变为黑色

    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;

    this->get_window()->get_geometry(x, y, width, height);
    auto root = this->get_screen()->get_root_window();
    auto pixbuf = Gdk::Pixbuf::create(root, x, y, width, height);

    KLOG_DEBUG("blur: %d-%d-%d-%d.", x, y, width, height);

    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, pixbuf->get_width(), pixbuf->get_height());
    auto cairo = Cairo::Context::create(surface);
    cairo->save();
    Gdk::Cairo::set_source_pixbuf(cairo, pixbuf, 0, 0);
    cairo->paint();
    cairo->restore();
    this->surface_blur(surface);
    return surface;
}

void ExitQueryWindow::pixel_blur(unsigned char pixel[],
                                 int32_t base[],
                                 int32_t channel_num)
{
    for (int32_t i = 0; i < channel_num; ++i)
    {
        auto tmp = base[i];
        base[i] += (this->blur_alpha_ * ((int32_t(pixel[i]) << BLUR_ZPREC) - tmp)) >> BLUR_APREC;
        pixel[i] = base[i] >> BLUR_ZPREC;
    }
}

void ExitQueryWindow::surface_blur(Cairo::RefPtr<Cairo::ImageSurface> surface)
{
    RETURN_IF_FALSE(surface->get_format() == Cairo::FORMAT_RGB24 ||
                    surface->get_format() == Cairo::FORMAT_ARGB32);

    auto pixels = surface->get_data();
    auto width = surface->get_width();
    auto height = surface->get_height();
    auto stride = surface->get_stride();
    auto channels = stride / width;
    int32_t origin[BLUR_MAX_CHANNEL_NUM] = {0};

    for (auto row = 0; row < height; ++row)
    {
        auto row_head = &pixels[row * stride];

        for (auto i = 0; i < channels; ++i)
        {
            origin[i] = *(row_head + i) << BLUR_ZPREC;
        }

        for (auto col = 0; col < width; col++)
        {
            this->pixel_blur(&row_head[col * channels], origin, channels);
        }

        for (auto col = width - 2; col >= 0; --col)
        {
            this->pixel_blur(&row_head[col * channels], origin, channels);
        }
    }

    for (auto col = 0; col < width; ++col)
    {
        auto col_head = pixels + col * channels;

        for (auto i = 0; i < channels; ++i)
        {
            origin[i] = *(col_head + i) << BLUR_ZPREC;
        }

        for (auto row = 0; row < height; ++row)
        {
            this->pixel_blur(&col_head[row * stride], origin, channels);
        }

        for (auto row = height - 2; row >= 0; --row)
        {
            this->pixel_blur(&col_head[row * stride], origin, channels);
        }
    }
}

void ExitQueryWindow::on_monitor_changed()
{
    // 窗口需要覆盖整个屏幕
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;

    this->get_screen()->get_root_window()->get_geometry(x, y, width, height);
    this->move(x, y);
    this->resize(width, height);
}

void ExitQueryWindow::on_content_size_allocate_cb(Gtk::Allocation &allocation)
{
    // 内容部分相对主显示器居中
    Gdk::Rectangle monitor_rect;
    auto primary_monitor = this->get_display()->get_primary_monitor();
    primary_monitor->get_geometry(monitor_rect);

    KLOG_DEBUG("Allocation      x-y-width-height: %d-%d-%d-%d.", allocation.get_x(), allocation.get_y(), allocation.get_width(), allocation.get_height());
    KLOG_DEBUG("Primary monitor x-y-width-height: %d-%d-%d-%d.", monitor_rect.get_x(), monitor_rect.get_y(), monitor_rect.get_width(), monitor_rect.get_height());

    allocation.set_x(monitor_rect.get_x() + (monitor_rect.get_width() - allocation.get_width()) / 2);
    allocation.set_y(monitor_rect.get_y() + (monitor_rect.get_height() - allocation.get_height()) / 2);
    this->content_->size_allocate(allocation);
}

}  // namespace Daemon

}  // namespace Kiran
