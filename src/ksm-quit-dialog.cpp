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

#include "src/ksm-quit-dialog.h"

namespace Kiran
{
#define GRESOURCE_PATH "/com/kylinsec/Kiran/SessionManager"

// KSMQuitDialog::KSMQuitDialog(KSMPowerAction power_action) : power_action_(power_action)
KSMQuitDialog::KSMQuitDialog(GtkDialog *dialog,
                             const Glib::RefPtr<Gtk::Builder> &builder,
                             KSMPowerAction power_action) : power_action_(power_action)
{
    this->init();
}

std::shared_ptr<KSMQuitDialog> KSMQuitDialog::create(KSMPowerAction power_action)
{
    KSMQuitDialog *dialog;
    auto builder = Gtk::Builder::create_from_resource(GRESOURCE_PATH "/ui/kiran-session-manager");
    builder->get_widget_derived("quit-dialog", dialog, power_action);
    return std::shared_ptr<KSMQuitDialog>(dialog);
}

void KSMQuitDialog::init()
{
}
}  // namespace Kiran
