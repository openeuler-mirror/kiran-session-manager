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

#include <glibmm/random.h>
#include "lib/base.h"

namespace Kiran
{
namespace Daemon
{
class Utils
{
public:
    Utils(){};
    virtual ~Utils(){};

    // 生成随机的startup id
    static std::string generate_startup_id();
    // 获取自动启动程序的目录列表
    static std::vector<std::string> get_autostart_dirs();
    // 设置环境变量
    static void setenv(const std::string &key, const std::string &value);
    static void setenvs(const std::map<Glib::ustring, Glib::ustring> &envs);
    // 生成cookie
    static uint32_t generate_cookie();
    //
    static KSMPhase phase_str2enum(const std::string &phase_str);
    static std::string phase_enum2str(KSMPhase phase);

private:
    static void setenv_to_dbus(const std::string &key, const std::string &value);
    static void setenv_to_systemd(const std::string &key, const std::string &value);

private:
    static Glib::Rand rand_;
};
}  // namespace Daemon
}  // namespace Kiran