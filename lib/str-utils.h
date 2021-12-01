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

#include "lib/base.h"

namespace fmt
{
template <>
struct formatter<Glib::ustring>
{
    template <typename ParseContext>
    constexpr typename ParseContext::iterator parse(ParseContext &ctx) const
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    format_context::iterator format(const Glib::ustring &str, FormatContext &ctx)
    {
        return format_to(ctx.out(), "{0}", str.raw());
    }
};
}  // namespace fmt

namespace Kiran
{
class StrUtils
{
public:
    StrUtils(){};
    virtual ~StrUtils(){};

    static std::vector<std::string> split_lines(const std::string &s);
    static std::string tolower(const std::string &str);
    static std::string toupper(const std::string &str);
    static std::vector<std::string> split_with_char(const std::string &s, char delimiter, bool is_merge_delimiter = false);

    // 去掉字符串前后的空白字符
    static std::string ltrim(const std::string &s);
    static std::string rtrim(const std::string &s);
    static std::string trim(const std::string &s);

    // 判断str是否以prefix字符串开头
    static bool startswith(const std::string &str, const std::string &prefix);
    static bool endswith(const std::string &str, const std::string &suffix);

    template <class T>
    static std::string join(const std::vector<T> &vec, const std::string &join_chars);
};

template <class T>
std::string StrUtils::join(const std::vector<T> &vec, const std::string &join_chars)
{
    std::string str;
    for (size_t i = 0; i < vec.size(); ++i)
    {
        str += fmt::format("{0}", vec[i]);
        if (i + 1 < vec.size())
        {
            str += fmt::format("{0}", join_chars);
        }
    }
    return str;
}
}  // namespace Kiran