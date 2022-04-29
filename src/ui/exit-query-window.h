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

#include <gtkmm.h>

#include "src/power.h"

namespace Kiran
{
namespace Daemon
{
enum ExitQueryResponse
{
    EXIT_QUERY_RESPONSE_OK,
    EXIT_QUERY_RESPONSE_CANCEL,
};

class ExitQueryWindow : public Gtk::Window
{
public:
    ExitQueryWindow(GtkWindow *window, const Glib::RefPtr<Gtk::Builder> &builder, PowerAction power_action);
    virtual ~ExitQueryWindow(){};

    static std::shared_ptr<ExitQueryWindow> create(PowerAction power_action);

    // 确认退出
    sigc::signal<void, ExitQueryResponse> signal_response() { return this->response_; };

protected:
    virtual void on_realize() override;
    virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;

private:
    void init();

    Cairo::RefPtr<Cairo::ImageSurface> get_blur_background_surface();

    // 高斯模糊
    void surface_blur(Cairo::RefPtr<Cairo::ImageSurface> surface);
    void pixel_blur(unsigned char pixel[], int32_t base[], int32_t channel_num);

    void on_monitor_changed();
    void on_content_size_allocate_cb(Gtk::Allocation &allocation);

private:
    Glib::RefPtr<Gtk::Builder> builder_;
    sigc::signal<void, ExitQueryResponse> response_;

    PowerAction power_action_;

    // 高斯模糊背景
    Cairo::RefPtr<Cairo::ImageSurface> background_surface_;
    int32_t blur_alpha_;

    // 内容
    Gtk::Box *content_;
    // 标题
    Gtk::Label *title_;
    // 标题描述
    Gtk::Label *title_desc_;
    // 抑制器列表
    Gtk::Box *inhibitors_;
    // 确认按钮
    Gtk::Button *ok_;
    // 取消按钮
    Gtk::Button *cancel_;
};

}  // namespace Daemon

}  // namespace Kiran
