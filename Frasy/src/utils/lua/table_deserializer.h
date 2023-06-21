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
#ifndef FRASY_SRC_UTILS_LUA_TABLE_DESERIALIZER_H
#define FRASY_SRC_UTILS_LUA_TABLE_DESERIALIZER_H

#include "utils/commands/description/value.h"
#include "utils/commands/type/enum.h"
#include "utils/commands/type/struct.h"
#include "utils/misc/deserializer.h"

#include <sol/sol.hpp>
#include <type_traits>
#include <vector>

namespace Frasy::Lua
{
template<typename Field>
    requires((
               requires(Field t) { t.Pos; } || requires(Field t) { t.Name; }) &&
             std::same_as<decltype(std::declval<Field>().Type), type_id_t> &&
             std::same_as<decltype(std::declval<Field>().Count), uint16_t>)
class Deserializer
{
    template<typename T>
    std::string GetPosOrName(const T& field)
    {
        return field.Name;
    }

    template<typename T>
        requires requires(T t) { t.Pos; }
    int GetPosOrName(const T& field)
    {
        return field.Pos;
    }

public:
    Deserializer(sol::state_view                                    lua,
                 const std::vector<Field>&                          fields,
                 const std::unordered_map<type_id_t, Type::Struct>& structs,
                 const std::unordered_map<type_id_t, Type::Enum>&   enums)
    : m_lua(lua), m_fields(fields), m_structs(structs), m_enums(enums)
    {
    }

    sol::table Deserialize(auto& begin, auto& end)
    {
        // Pre-allocate memory for m_fields.size() hashable entries in the table.
        sol::table table = m_lua.create_table(static_cast<int>(m_fields.size()), static_cast<int>(m_fields.size()));

        for (auto&& field : m_fields) { ParseFieldAndAddToTable(table, field, begin, end); }

        return table;
    }

private:
    sol::state_view                                    m_lua;
    const std::vector<Field>&                          m_fields;
    const std::unordered_map<type_id_t, Type::Struct>& m_structs;
    const std::unordered_map<type_id_t, Type::Enum>&   m_enums;


private:
    void ParseFieldAndAddToTable(sol::table& table, const Field& field, auto& begin, const auto& end)
    {
        switch (field.Type)
        {
            case static_cast<type_id_t>(Type::Fundamental::E::Bool):
                ParseFieldAndAddToTableImpl<bool>(table, field, begin, end);
                break;
            case static_cast<type_id_t>(Type::Fundamental::E::Int8):
                ParseFieldAndAddToTableImpl<int8_t>(table, field, begin, end);
                break;
            case static_cast<type_id_t>(Type::Fundamental::E::UInt8):
                ParseFieldAndAddToTableImpl<uint8_t>(table, field, begin, end);
                break;
            case static_cast<type_id_t>(Type::Fundamental::E::Int16):
                ParseFieldAndAddToTableImpl<int16_t>(table, field, begin, end);
                break;
            case static_cast<type_id_t>(Type::Fundamental::E::UInt16):
                ParseFieldAndAddToTableImpl<uint16_t>(table, field, begin, end);
                break;
            case static_cast<type_id_t>(Type::Fundamental::E::Int32):
                ParseFieldAndAddToTableImpl<int32_t>(table, field, begin, end);
                break;
            case static_cast<type_id_t>(Type::Fundamental::E::UInt32):
                ParseFieldAndAddToTableImpl<uint32_t>(table, field, begin, end);
                break;
            case static_cast<type_id_t>(Type::Fundamental::E::Int64):
                ParseFieldAndAddToTableImpl<int64_t>(table, field, begin, end);
                break;
            case static_cast<type_id_t>(Type::Fundamental::E::UInt64):
                ParseFieldAndAddToTableImpl<uint64_t>(table, field, begin, end);
                break;
            case static_cast<type_id_t>(Type::Fundamental::E::Float):
                ParseFieldAndAddToTableImpl<float>(table, field, begin, end);
                break;
            case static_cast<type_id_t>(Type::Fundamental::E::Double):
                ParseFieldAndAddToTableImpl<double>(table, field, begin, end);
                break;
            case static_cast<type_id_t>(Type::Fundamental::E::String):
                ParseFieldAndAddToTableImpl<std::string>(table, field, begin, end);
                break;
            default:
                if (m_structs.contains(field.Type))
                {
                    if (field.Count == 1)
                    {
                        auto s = m_structs.at(field.Type);

                        Deserializer<typename decltype(s.Fields)::value_type> deserializer {
                          m_lua, s.Fields, m_structs, m_enums};
                        table[GetPosOrName(field)] = deserializer.Deserialize(begin, end);
                    }
                    else
                    {
                        uint16_t rcvdSize = Frasy::Deserialize<uint16_t>(begin, end);
                        if (field.Count != 0 && rcvdSize != field.Count)
                        {
                            throw std::runtime_error(
                              std::format("Not enough data received! Expected {}, got {}", field.Count, rcvdSize));
                        }
                        sol::table sTable = m_lua.create_table(field.Count, 0);
                        for (size_t i = 0; i < rcvdSize; i++)
                        {
                            auto s = m_structs.at(field.Type);

                            Deserializer<typename decltype(s.Fields)::value_type> deserializer {
                              m_lua, s.Fields, m_structs, m_enums};
                            sTable.add(deserializer.Deserialize(begin, end));
                        }
                        table[GetPosOrName(field)] = sTable;
                    }
                }
                else if (m_enums.contains(field.Type))
                {
                    switch (m_enums.at(field.Type).UnderlyingSize)
                    {
                        case 1: ParseFieldAndAddToTableImpl<uint8_t>(table, field, begin, end); break;
                        case 2: ParseFieldAndAddToTableImpl<uint16_t>(table, field, begin, end); break;
                        case 4: ParseFieldAndAddToTableImpl<uint32_t>(table, field, begin, end); break;
                        default: table[GetPosOrName(field)] = sol::nil; break;
                    }
                }
                else
                {
                    BR_LOG_ERROR("Deserializer", "Unknown type {}", field.Type);
                    table[GetPosOrName(field)] = sol::nil;
                }
                break;
        }
    }

    template<typename T>
    void ParseFieldAndAddToTableImpl(sol::table& table, const Field& field, auto& begin, const auto& end)
    {
        if (field.Count == 1)
        {
            T t                        = Frasy::Deserialize<T>(begin, end);
            table[GetPosOrName(field)] = t;
        }
        else
        {
            std::vector<T> t           = Frasy::Deserialize<std::vector<T>>(begin, end);
            table[GetPosOrName(field)] = t;
        }
    }
};

}    // namespace Frasy::Lua

#endif    // FRASY_SRC_UTILS_LUA_TABLE_DESERIALIZER_H
