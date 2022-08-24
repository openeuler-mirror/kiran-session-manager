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


#include <map>
#include <vector>

namespace Kiran
{
class MapHelper
{
public:
    MapHelper(){};
    virtual ~MapHelper(){};

    template <typename K, typename V>
    static std::vector<V> get_values(const std::map<K, V> &maps)
    {
        std::vector<V> values;
        for (auto &iter : maps)
        {
            values.push_back(iter.second);
        }
        return values;
    }

    template <typename K, typename V>
    static V get_value(const std::map<K, V> &maps, const K &key)
    {
        auto iter = maps.find(key);
        if (iter != maps.end())
        {
            return iter->second;
        }
        return nullptr;
    }
};

}  // namespace Kiran