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

namespace Kiran
{
namespace Daemon
{
class ExitQueryWindow;

class UIManager
{
public:
    UIManager();
    virtual ~UIManager(){};

    static UIManager* get_instance() { return instance_; };

    static void global_init();

    static void global_deinit() { delete instance_; };

private:
    void init();

private:
    static UIManager* instance_;

    // std::shared_ptr<MaskWindow> mask_window_;
};
}  // namespace Daemon

}  // namespace Kiran
