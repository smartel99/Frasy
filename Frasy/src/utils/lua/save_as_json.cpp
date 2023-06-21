/**
 * @file    save_as_json.cpp
 * @author  Paul Thomas
 * @date    5/3/2023
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
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#include "save_as_json.h"

#include "Brigerad/Core/Log.h"

#include <format>
#include <fstream>
#include <json.hpp>

namespace Frasy::Lua
{
namespace
{
using json = nlohmann::json;

template<typename T>
void AddToJson(json& j, sol::object key, sol::object value)    // NOLINT(performance-unnecessary-value-param)
{
    if (j.is_array()) { j += value.as<T>(); }
    else { j[key.as<std::string>()] = value.as<T>(); }
}

json MakeJson(sol::table table)    // NOLINT(performance-unnecessary-value-param)
{
    json      j;
    sol::type key_type    = sol::type::none;
    sol::type object_type = sol::type::none;
    for (auto [key, value] : table)
    {
        if (key_type == sol::type::none)
        {
            key_type = key.get_type();
            if (key_type == sol::type::number) { j = json::array(); }
            else if (key_type == sol::type::string) { j = json::object(); }
            else
            {
                throw std::runtime_error(
                  std::format("Invalid key, not a string nor a number. Type: {}", (int)key_type));
            }
        }
        if (key.get_type() != key_type)
        {
            throw std::runtime_error(std::format(
              "Key type has changed on same level. Expected: {}, Got: {}", (int)key_type, (int)key.get_type()));
        }
        if (key_type == sol::type::number)
        {
            if (object_type == sol::type::none) { object_type = value.get_type(); }
            if (object_type != value.get_type())
            {
                throw std::runtime_error(std::format(
                  "Object type has changed while in an array. Expected: {}, Got: {}",
                  (int)key_type,
                  (int)key.get_type()));
            }
        }
        switch (value.get_type())
        {
            case sol::type::string: AddToJson<std::string>(j, key, value); break;
            case sol::type::number: AddToJson<double>(j, key, value); break;
            case sol::type::boolean: AddToJson<bool>(j, key, value); break;
            case sol::type::table:
            case sol::type::userdata:
            case sol::type::lightuserdata:
            {
                if (j.is_array()) { j += MakeJson(value); }
                else { j[key.as<std::string>()] = MakeJson(value); }
                break;
            }
            case sol::type::thread:
            case sol::type::poly:
            case sol::type::function:
            case sol::type::none:
            case sol::type::lua_nil:
                throw std::runtime_error("Object is not jsonable. Type: " + std::to_string((int)value.get_type()));
        }
    }
    return j;
}
}    // namespace

void SaveAsJson(sol::table table, const std::string& file)
{
    try
    {
        json          object = MakeJson(table);    // NOLINT(performance-unnecessary-value-param)
        std::ofstream o      = std::ofstream(file);
        o << object.dump(2, ' ', true);
        o.close();
    }
    catch (const std::runtime_error& e)
    {
        BR_LUA_ERROR("Failed to save table as json. Reason: {}", e.what());
    }
}


}    // namespace Frasy::Lua
