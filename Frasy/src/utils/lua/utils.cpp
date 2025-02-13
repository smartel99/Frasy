/**
 * @file    utils.cpp
 * @author  Paul Thomas
 * @date    3/21/2023
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "utils.h"

#include <iostream>

sol::object copy(const sol::object& obj, sol::state_view target)
{
    sol::type tp = obj.get_type();
    if (tp == sol::type::number) {
        const double d = obj.as<double>();
        return sol::make_object(target, d);
    }
    else if (tp == sol::type::boolean) {
        const bool b = obj.as<bool>();
        return sol::make_object(target, b);
    }
    else if (tp == sol::type::string) {
        const std::string s = obj.as<std::string>();
        return sol::make_object(target, s);
    }
    else if (tp == sol::type::table) {
        sol::table t     = obj.as<sol::table>();
        sol::table tcopy = target.create_table();
        for (auto& [k, v] : t) {
            tcopy.set(copy(k, target), copy(v, target));
        }
        return tcopy;
    }
    return {};
}

void print(const sol::object& obj, int level)
{
    sol::type tp = obj.get_type();
    for (int i = 0; i < level - 1; ++i) {
        std::cout << "   ";
    }
    if (level != 0) { std::cout << "|--- "; }

    auto pv = [](const sol::object& o) {
        sol::type tp = o.get_type();
        if (tp == sol::type::number) { std::cout << std::to_string(o.as<double>()); }
        else if (tp == sol::type::boolean) {
            std::cout << std::to_string(o.as<bool>());
        }
        else if (tp == sol::type::string) {
            std::cout << o.as<std::string>();
        }
        else {
            std::cout << "Invalid type: " << static_cast<int>(tp);
        }
    };

    if (tp == sol::type::table) {
        sol::table t = obj.as<sol::table>();
        for (auto [key, val] : t) {
            pv(key);
            std::cout << ":" << std::endl;
            print(val, level + 1);
        }
    }
    else {
        pv(obj);
        std::cout << std::endl;
    }
}
