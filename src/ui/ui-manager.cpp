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

#include "src/ui/ui-manager.h"
#include "lib/base.h"

namespace Kiran
{
namespace Daemon
{
UIManager::UIManager()
{
}

UIManager *UIManager::instance_ = nullptr;
void UIManager::global_init()
{
    instance_ = new UIManager();
    instance_->init();
}

void UIManager::init()
{
    KLOG_PROFILE("");

    auto resource = Gio::Resource::create_from_file(KSM_INSTALL_DATADIR "/kiran-session-manager.gresource");
    resource->register_global();

    try
    {
        auto provider = Gtk::CssProvider::create();
        provider->load_from_resource(GRESOURCE_PATH "/css/main");
        Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(),
                                                   provider,
                                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    catch (const Glib::Error &e)
    {
        KLOG_ERROR("Failed to load style file: '%s'", e.what().c_str());
        return;
    }
}
}  // namespace Daemon

}  // namespace Kiran
