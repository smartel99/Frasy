/**
 * @file    obj2str.cpp
 * @author  Paul Thomas
 * @date    3/18/2025
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

#include "obj2str.h"

namespace Frasy::Report {
auto obj2str(const sol::object& object) -> std::string
{
    switch (object.get_type()) {
        case sol::type::none: return "none";
        case sol::type::nil: return "nil";
        case sol::type::string: return object.as<std::string>();
        case sol::type::number: return std::to_string(object.as<double>());
        case sol::type::thread: return std::format("thread: {}", object.pointer());
        case sol::type::boolean: return "boolean";
        case sol::type::function: return std::format("function: {}", object.pointer());
        case sol::type::userdata: return std::format("userdata: {}", object.pointer());
        case sol::type::lightuserdata: return std::format("lightuserdata", object.pointer());
        case sol::type::table: return std::format("table: {}", object.pointer());
        case sol::type::poly: return "poly";
        default: return "unknown";
    }
}
}
