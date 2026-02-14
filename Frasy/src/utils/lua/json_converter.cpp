/**
 * @file    json_converter.cpp
 * @author  Paul Thomas
 * @date    3/17/2025
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

#include "json_converter.h"

namespace Frasy::Lua {

#pragma region table to json
namespace {
nlohmann::json tableToJsonArray(const sol::table& table)
{
    auto       array = nlohmann::json::array();
    const auto size  = table.size();
    for (std::size_t i = 0; i < size; ++i) {
        switch (const auto object = table[i]; object.get_type()) {
            case sol::type::none:
            case sol::type::nil: array.push_back("nil");
                break;
            case sol::type::string: array.push_back(object.get<std::string>());
                break;
            case sol::type::number: array.push_back(object.get<double>());
                break;
            case sol::type::thread: array.push_back("thread");
                break;
            case sol::type::boolean: array.push_back(object.get<bool>());
                break;
            case sol::type::function: array.push_back("function");
                break;
            case sol::type::userdata: array.push_back("userdata");
                break;
            case sol::type::lightuserdata: array.push_back("lightuserdata");
                break;
            case sol::type::table: array.push_back(tableToJson(object));
                break;
            case sol::type::poly: array.push_back("poly");
                break;
        }
    }
    return array;
}

nlohmann::json tableToJsonObject(const sol::table& table)
{
    auto object = nlohmann::json::object();
    for (const auto& [keyObj, value] : table) {
        const auto key = keyObj.as<std::string>();
        switch (value.get_type()) {
            case sol::type::none:
            case sol::type::nil: object[key] = "nil";
                break;
            case sol::type::string: object[key] = value.as<std::string>();
                break;
            case sol::type::number: object[key] = value.as<double>();
                break;
            case sol::type::thread: object[key] = std::format("thread: {}", value.pointer());
                break;
            case sol::type::boolean: object[key] = value.as<bool>();
                break;
            case sol::type::function: object[key] = std::format("function: {}", value.pointer());
                break;
            case sol::type::userdata: object[key] = std::format("userdata: {}", value.pointer());
                break;
            case sol::type::lightuserdata: object[key] = std::format("lightuserdata: {}", value.pointer());
                break;
            case sol::type::table: object[key] = tableToJson(value.as<sol::table>());
                break;
            case sol::type::poly: object[key] = std::format("poly: {}", value.pointer());
                break;
        }
    }
    return object;
}
}

nlohmann::json tableToJson(const sol::table& table)
{
    return table.size() == 0 ? tableToJsonObject(table) : tableToJsonArray(table);
}

#pragma endregion

#pragma region json to table
namespace {
sol::table jsonArrayToTable(sol::state& lua, const nlohmann::json& array);
sol::table jsonObjectToTable(sol::state& lua, const nlohmann::json& object);

sol::table jsonArrayToTable(sol::state& lua, const nlohmann::json& array)
{
    auto table = lua.create_table();
    for (const auto& [index, value] : array.items()) {
        switch (value.type()) {
            case nlohmann::detail::value_t::null: table.add(sol::nil);
                break;
            case nlohmann::detail::value_t::object: table.add(jsonObjectToTable(lua, value));
                break;
            case nlohmann::detail::value_t::array: table.add(jsonArrayToTable(lua, value));
                break;
            case nlohmann::detail::value_t::string: table.add(value.get<std::string>());
                break;
            case nlohmann::detail::value_t::boolean: table.add(value.get<bool>());
                break;
            case nlohmann::detail::value_t::number_integer: table.add(value.get<int32_t>());
                break;
            case nlohmann::detail::value_t::number_unsigned: table.add(value.get<uint32_t>());
                break;
            case nlohmann::detail::value_t::number_float: table.add(value.get<double>());
                break;
            case nlohmann::detail::value_t::discarded: table.add(sol::nil);
                break;
            case nlohmann::detail::value_t::binary: // TODO Maybe do something with binary data??
                break;
            default: break;
        }
    }
    return table;
}

sol::table jsonObjectToTable(sol::state& lua, const nlohmann::json& object)
{
    auto table = lua.create_table();
    for (const auto& [key, value] : object.items()) {
        switch (value.type()) {
            case nlohmann::detail::value_t::null: table[key] = sol::nil;
                break;
            case nlohmann::detail::value_t::object: table[key] = jsonObjectToTable(lua, value);
                break;
            case nlohmann::detail::value_t::array: table[key] = jsonArrayToTable(lua, value);
                break;
            case nlohmann::detail::value_t::string: table[key] = value.get<std::string>();
                break;
            case nlohmann::detail::value_t::boolean: table[key] = value.get<bool>();
                break;
            case nlohmann::detail::value_t::number_integer: table[key] = value.get<int32_t>();
                break;
            case nlohmann::detail::value_t::number_unsigned: table[key] = value.get<uint32_t>();
                break;
            case nlohmann::detail::value_t::number_float: table[key] = value.get<double>();
                break;
            case nlohmann::detail::value_t::discarded: table[key] = sol::nil;
                break;
            case nlohmann::detail::value_t::binary: // TODO Maybe do something with binary data??
                break;
            default: break;
        }

    }
    return table;
}
}

sol::table jsonToTable(sol::state& lua, const nlohmann::json& json)
{
    return json.is_object() ? jsonObjectToTable(lua, json) : jsonArrayToTable(lua, json);

}

#pragma endregion

}
