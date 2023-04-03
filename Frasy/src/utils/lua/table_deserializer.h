/**
 * @file    table_deserializer.h
 * @author  Paul Thomas
 * @date    2023-03-06
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
#ifndef COPY_LUA_PY_FRASY_SRC_UTILS_LUA_TABLE_DESERIALIZER_H
#define COPY_LUA_PY_FRASY_SRC_UTILS_LUA_TABLE_DESERIALIZER_H

#include "utils/commands/description/value.h"
#include "utils/commands/type/enum.h"
#include "utils/commands/type/struct.h"

#include <sol/sol.hpp>
#include <vector>
namespace Frasy::Lua
{
sol::table Deserialize(sol::state&                                        lua,
                       const std::vector<Type::Struct::Field>&            fields,
                       const std::unordered_map<type_id_t, Type::Struct>& structs,
                       const std::unordered_map<type_id_t, Type::Enum>&   enums,
                       const std::vector<uint8_t>&                        data);

sol::table Deserialize(sol::state&                                        lua,
                       const std::vector<Type::Struct::Field>&            fields,
                       const std::unordered_map<type_id_t, Type::Struct>& structs,
                       const std::unordered_map<type_id_t, Type::Enum>&   enums,
                       const std::vector<uint8_t>&                        data,
                       std::size_t&                                       offset);

sol::table Deserialize(sol::state&                                        lua,
                       const std::vector<Frasy::Actions::Value>           values,
                       const std::unordered_map<type_id_t, Type::Struct>& structs,
                       const std::unordered_map<type_id_t, Type::Enum>&   enums,
                       const std::vector<uint8_t>&                        data);

sol::table Deserialize(sol::state&                                        lua,
                       const std::vector<Frasy::Actions::Value>           values,
                       const std::unordered_map<type_id_t, Type::Struct>& structs,
                       const std::unordered_map<type_id_t, Type::Enum>&   enums,
                       const std::vector<uint8_t>&                        data,
                       std::size_t&                                       offset);

}    // namespace Frasy::Lua

#endif    // COPY_LUA_PY_FRASY_SRC_UTILS_LUA_TABLE_DESERIALIZER_H
