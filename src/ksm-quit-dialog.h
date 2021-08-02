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

#pragma once

#include <gtkmm.h>
#include "src/ksm-power.h"

namespace Kiran
{
class KSMQuitDialog : public Gtk::Dialog
{
public:
    KSMQuitDialog(GtkDialog *dialog, const Glib::RefPtr<Gtk::Builder> &builder, KSMPowerAction power_action);
    // KSMQuitDialog(KSMPowerAction power_action);
    virtual ~KSMQuitDialog(){};

    static std::shared_ptr<KSMQuitDialog> create(KSMPowerAction power_action);

    KSMPowerAction get_power_action() { return this->power_action_; };

private:
    void init();

private:
    KSMPowerAction power_action_;
};

}  // namespace Kiran
