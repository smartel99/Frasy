/**
 * @file    table_deserializer.cpp
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

#include "table_deserializer.h"

#include "tag.h"
#include "utils/misc/deserializer.h"

namespace Frasy::Lua
{
sol::table Deserialize(sol::state&                                        lua,
                       const std::vector<Type::Struct::Field>&            fields,
                       const std::unordered_map<type_id_t, Type::Struct>& structs,
                       const std::unordered_map<type_id_t, Type::Enum>&   enums,
                       const std::vector<uint8_t>&                        data)
{
    std::size_t offset = 0;
    return Deserialize(lua, fields, structs, enums, data, offset);
}

sol::table Deserialize(sol::state&                                        lua,
                       const std::vector<Type::Struct::Field>&            fields,
                       const std::unordered_map<type_id_t, Type::Struct>& structs,
                       const std::unordered_map<type_id_t, Type::Enum>&   enums,
                       const std::vector<uint8_t>&                        data,
                       std::size_t&                                       offset)
{
    sol::table table = lua.create_table();

    auto fSingle =
      []<typename T>(sol::table& table, const std::string& name, const std::vector<uint8_t>& data, std::size_t& offset)
    {
        table[name] = Frasy::Deserialize<T>(data.begin() + offset, data.end());
        offset += sizeof(T);
    };

    auto fVector = []<typename T>(sol::state&                 lua,
                                  sol::table&                 table,
                                  const std::string&          name,
                                  const std::vector<uint8_t>& data,
                                  std::size_t&                offset)
    {
        auto v = Frasy::Deserialize<std::vector<T>>(data.begin() + offset, data.end());
        offset += v.size() * sizeof(T) + sizeof(Frasy::serializable_container_size_t);
        table[name] = sol::as_table(v);
    };

    auto fString = [](sol::table& table, const std::string& name, const std::vector<uint8_t>& data, std::size_t& offset)
    {
        auto str = Frasy::Deserialize<std::string>(data.begin() + offset, data.end());
        offset += sizeof(char) * str.size() + sizeof(serializable_container_size_t);
        table[name] = str;
    };

    auto fStringVector = [](sol::state&                 lua,
                            sol::table&                 table,
                            const std::string&          name,
                            const std::vector<uint8_t>& data,
                            std::size_t&                offset)
    {
        auto strs = Frasy::Deserialize<std::vector<std::string>>(data.begin() + offset, data.end());
        offset += sizeof(serializable_container_size_t);
        table[name] = sol::as_table(strs);
        for (const auto& str : strs) { offset += sizeof(serializable_container_size_t) + sizeof(char) * str.size(); }
    };

    for (const auto& field : fields)
    {
        if (field.Count == 1)
        {
            switch (field.Type)
            {
                case static_cast<type_id_t>(Type::Fundamental::E::Bool):
                    fSingle.operator()<bool>(table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int8):
                    fSingle.operator()<int8_t>(table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt8):
                    fSingle.operator()<uint8_t>(table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int16):
                    fSingle.operator()<int16_t>(table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt16):
                    fSingle.operator()<uint16_t>(table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int32):
                    fSingle.operator()<int32_t>(table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt32):
                    fSingle.operator()<uint32_t>(table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int64):
                    fSingle.operator()<int64_t>(table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt64):
                    fSingle.operator()<uint64_t>(table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Float):
                    fSingle.operator()<float>(table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Double):
                    fSingle.operator()<double>(table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::String):
                    fString(table, field.Name, data, offset);
                    break;
                default:
                    if (structs.contains(field.Type))
                    {
                        const auto& s     = structs.at(field.Type);
                        table[field.Name] = Deserialize(lua, s.Fields, structs, enums, data, offset);
                    }
                    else if (enums.contains(field.Type))
                    {
                        fSingle.operator()<uint32_t>(table, field.Name, data, offset);
                    }
                    else { BR_LOG_ERROR(s_tag.data(), "Unknown type {}", field.Type); }
                    break;
            }
        }
        else
        {
            switch (field.Type)
            {
                case static_cast<type_id_t>(Type::Fundamental::E::Bool):
                    fVector.operator()<bool>(lua, table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int8):
                    fVector.operator()<int8_t>(lua, table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt8):
                    fVector.operator()<uint8_t>(lua, table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int16):
                    fVector.operator()<int16_t>(lua, table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt16):
                    fVector.operator()<uint16_t>(lua, table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int32):
                    fVector.operator()<int32_t>(lua, table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt32):
                    fVector.operator()<uint32_t>(lua, table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int64):
                    fVector.operator()<int64_t>(lua, table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt64):
                    fVector.operator()<uint64_t>(lua, table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Float):
                    fVector.operator()<float>(lua, table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Double):
                    fVector.operator()<double>(lua, table, field.Name, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::String):
                    fStringVector(lua, table, field.Name, data, offset);
                    break;
                default:
                    if (structs.contains(field.Type))
                    {
                        const auto& st       = structs.at(field.Type);
                        auto        subtable = lua.create_table();
                        auto        size =
                          Frasy::Deserialize<serializable_container_size_t>(data.begin() + offset, data.end());
                        offset += sizeof(serializable_container_size_t);
                        for (int i = 0; i < size; ++i)
                        {
                            subtable.add(Deserialize(lua, st.Fields, structs, enums, data, offset));
                        }
                        table[field.Name] = subtable;
                    }
                    else if (enums.contains(field.Type))
                    {
                        fVector.operator()<uint32_t>(lua, table, field.Name, data, offset);
                    }
                    else { BR_LOG_ERROR(s_tag.data(), "Unknown type {}", field.Type); }
                    break;
            }
        }
    }
    return table;
}

sol::table Deserialize(sol::state&                                        lua,
                       const std::vector<Frasy::Actions::Value>           values,
                       const std::unordered_map<type_id_t, Type::Struct>& structs,
                       const std::unordered_map<type_id_t, Type::Enum>&   enums,
                       const std::vector<uint8_t>&                        data)
{
    std::size_t offset = 0;
    return Deserialize(lua, values, structs, enums, data, offset);
}

sol::table Deserialize(sol::state&                                        lua,
                       const std::vector<Frasy::Actions::Value>           values,
                       const std::unordered_map<type_id_t, Type::Struct>& structs,
                       const std::unordered_map<type_id_t, Type::Enum>&   enums,
                       const std::vector<uint8_t>&                        data,
                       std::size_t&                                       offset)
{
    sol::table table = lua.create_table();

    auto fSingle =
      []<typename T>(sol::table& table, std::size_t pos, const std::vector<uint8_t>& data, std::size_t& offset)
    {
        table[pos + 1] = Frasy::Deserialize<T>(data.begin() + offset, data.end());
        offset += sizeof(T);
    };

    auto fVector =
      []<typename T>(
        sol::state& lua, sol::table& table, std::size_t pos, const std::vector<uint8_t>& data, std::size_t& offset)
    {
        auto v = Frasy::Deserialize<std::vector<T>>(data.begin() + offset, data.end());
        offset += v.size() * sizeof(T) + sizeof(Frasy::serializable_container_size_t);
        table[pos + 1] = sol::as_table(v);
    };

    auto fString = [](sol::table& table, std::size_t pos, const std::vector<uint8_t>& data, std::size_t& offset)
    {
        auto str = Frasy::Deserialize<std::string>(data.begin() + offset, data.end());
        offset += sizeof(char) * str.size() + sizeof(serializable_container_size_t);
        table[pos + 1] = str;
    };

    auto fStringVector =
      [](sol::state& lua, sol::table& table, std::size_t pos, const std::vector<uint8_t>& data, std::size_t& offset)
    {
        auto strs = Frasy::Deserialize<std::vector<std::string>>(data.begin() + offset, data.end());
        offset += sizeof(serializable_container_size_t);
        table[pos + 1] = sol::as_table(strs);
        for (const auto& str : strs) { offset += sizeof(serializable_container_size_t) + sizeof(char) * str.size(); }
    };

    for (const auto& value : values)
    {
        if (value.Count == 1)
        {
            switch (value.Type)
            {
                case static_cast<type_id_t>(Type::Fundamental::E::Bool):
                    fSingle.operator()<bool>(table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int8):
                    fSingle.operator()<int8_t>(table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt8):
                    fSingle.operator()<uint8_t>(table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int16):
                    fSingle.operator()<int16_t>(table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt16):
                    fSingle.operator()<uint16_t>(table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int32):
                    fSingle.operator()<int32_t>(table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt32):
                    fSingle.operator()<uint32_t>(table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int64):
                    fSingle.operator()<int64_t>(table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt64):
                    fSingle.operator()<uint64_t>(table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Float):
                    fSingle.operator()<float>(table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Double):
                    fSingle.operator()<double>(table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::String):
                    fString(table, value.Pos, data, offset);
                    break;
                default:
                    if (structs.contains(value.Type))
                    {
                        const auto& s        = structs.at(value.Type);
                        table[value.Pos + 1] = Deserialize(lua, s.Fields, structs, enums, data, offset);
                    }
                    else if (enums.contains(value.Type))
                    {
                        fSingle.operator()<uint32_t>(table, value.Pos, data, offset);
                    }
                    else { BR_LOG_ERROR(s_tag.data(), "Unknown type {}", value.Type); }
                    break;
            }
        }
        else
        {
            switch (value.Type)
            {
                case static_cast<type_id_t>(Type::Fundamental::E::Bool):
                    fVector.operator()<bool>(lua, table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int8):
                    fVector.operator()<int8_t>(lua, table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt8):
                    fVector.operator()<uint8_t>(lua, table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int16):
                    fVector.operator()<int16_t>(lua, table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt16):
                    fVector.operator()<uint16_t>(lua, table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int32):
                    fVector.operator()<int32_t>(lua, table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt32):
                    fVector.operator()<uint32_t>(lua, table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Int64):
                    fVector.operator()<int64_t>(lua, table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::UInt64):
                    fVector.operator()<uint64_t>(lua, table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Float):
                    fVector.operator()<float>(lua, table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::Double):
                    fVector.operator()<double>(lua, table, value.Pos, data, offset);
                    break;
                case static_cast<type_id_t>(Type::Fundamental::E::String):
                    fStringVector(lua, table, value.Pos, data, offset);
                    break;
                default:
                    if (structs.contains(value.Type))
                    {
                        const auto& st       = structs.at(value.Type);
                        auto        subtable = lua.create_table();
                        auto        size =
                          Frasy::Deserialize<serializable_container_size_t>(data.begin() + offset, data.end());
                        offset += sizeof(serializable_container_size_t);
                        for (int i = 0; i < size; ++i)
                        {
                            subtable.add(Deserialize(lua, st.Fields, structs, enums, data, offset));
                        }
                        table[value.Pos + 1] = subtable;
                    }
                    else if (enums.contains(value.Type))
                    {
                        fVector.operator()<uint32_t>(lua, table, value.Pos, data, offset);
                    }
                    else { BR_LOG_ERROR(s_tag.data(), "Unknown type {}", value.Type); }
                    break;
            }
        }
    }
    return table;
}

}    // namespace Frasy::Lua
