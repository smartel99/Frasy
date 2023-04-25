/**
 * @file    table_serializer.cpp
 * @author  Paul Thomas
 * @date    2023-03-01
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

#include "table_serializer.h"

#include "tag.h"
#include "utils/commands/description/value.h"
#include "utils/commands/type/manager/manager.h"
#include "utils/misc/serializer.h"

#include <Brigerad/Core/Log.h>

namespace Frasy::Lua
{
using Type::Manager;
using Type::Struct;

void ArgsToTable(sol::table&                       table,
                 const Frasy::Type::Manager&       typeManager,
                 const std::vector<Struct::Field>& fields,
                 const sol::variadic_args&         args)
{
    decltype(auto) f = []<typename T>(const sol::variadic_args& args, int index) { return args.get<T>(index); };
    for (int i = 0; i < args.size(); ++i)
    {
        const auto& field = fields[i];
        if (field.Count != Type::SINGLE) { table[field.Name] = f.operator()<sol::table>(args, i); }
        else if (typeManager.IsFundamental(field.Type))
        {
            switch (static_cast<Type::Fundamental::E>(field.Type))
            {
                case Type::Fundamental::E::Bool: table[field.Name] = f.operator()<bool>(args, i); break;
                case Type::Fundamental::E::Int8: table[field.Name] = f.operator()<int8_t>(args, i); break;
                case Type::Fundamental::E::UInt8: table[field.Name] = f.operator()<uint8_t>(args, i); break;
                case Type::Fundamental::E::Int16: table[field.Name] = f.operator()<int16_t>(args, i); break;
                case Type::Fundamental::E::UInt16: table[field.Name] = f.operator()<uint16_t>(args, i); break;
                case Type::Fundamental::E::Int32: table[field.Name] = f.operator()<int32_t>(args, i); break;
                case Type::Fundamental::E::UInt32: table[field.Name] = f.operator()<uint32_t>(args, i); break;
                case Type::Fundamental::E::Int64: table[field.Name] = f.operator()<int64_t>(args, i); break;
                case Type::Fundamental::E::UInt64: table[field.Name] = f.operator()<uint64_t>(args, i); break;
                case Type::Fundamental::E::Float: table[field.Name] = f.operator()<float>(args, i); break;
                case Type::Fundamental::E::Double: table[field.Name] = f.operator()<double>(args, i); break;
                case Type::Fundamental::E::String: table[field.Name] = f.operator()<std::string>(args, i); break;

                case Type::Fundamental::E::Void:
                case Type::Fundamental::E::Size:
                default: BR_LOG_ERROR(s_tag.data(), "Invalid type"); throw std::exception();
            }
        }
        else if (typeManager.IsEnum(field.Type)) { table[field.Name] = f.operator()<uint32_t>(args, i); }
        else { table[field.Name] = f.operator()<sol::table>(args, i); }
    }
}

void ParseTable(const sol::table&                 table,
                const Type::Manager&              typeManager,
                const std::vector<Struct::Field>& fields,
                std::vector<uint8_t>&             output)
{
    for (const auto& field : fields)
    {
        if (field.Count != Type::SINGLE) { ParseContainer(table, typeManager, field, output); }
        else if (typeManager.IsFundamental(field.Type)) { ParseFundamental(table, field, output); }
        else if (typeManager.IsEnum(field.Type)) { ParseEnum(table, typeManager, field, output); }
        else
        {
            ParseTable(
              table.get<sol::table>(field.Name), typeManager, typeManager.GetStruct(field.Type).Fields, output);
        }
    }
}

void ParseFundamental(const sol::table& table, const Struct::Field& field, std::vector<uint8_t>& output)
{
    auto f = [&]<typename T>()
    {
        auto input = Serialize<T>(table.get<T>(field.Name));
        output.insert(output.end(), input.begin(), input.end());
    };

    switch (static_cast<Type::Fundamental::E>(field.Type))
    {
        case Type::Fundamental::E::Bool: f.operator()<bool>(); break;
        case Type::Fundamental::E::Int8: f.operator()<int8_t>(); break;
        case Type::Fundamental::E::UInt8: f.operator()<uint8_t>(); break;
        case Type::Fundamental::E::Int16: f.operator()<int16_t>(); break;
        case Type::Fundamental::E::UInt16: f.operator()<uint16_t>(); break;
        case Type::Fundamental::E::Int32: f.operator()<int32_t>(); break;
        case Type::Fundamental::E::UInt32: f.operator()<uint32_t>(); break;
        case Type::Fundamental::E::Int64: f.operator()<int64_t>(); break;
        case Type::Fundamental::E::UInt64: f.operator()<uint64_t>(); break;
        case Type::Fundamental::E::Float: f.operator()<float>(); break;
        case Type::Fundamental::E::Double: f.operator()<double>(); break;
        case Type::Fundamental::E::String: f.operator()<std::string>(); break;
        case Type::Fundamental::E::Void:
        case Type::Fundamental::E::Size:
        default: BR_LOG_ERROR(s_tag.data(), "Type is not a fundamental"); throw std::exception();
    }
}

void ParseEnum(const sol::table&     table,
               const Type::Manager&  typeManager,
               const Struct::Field&  field,
               std::vector<uint8_t>& output)
{
    using Type::Fundamental;
    auto to_underlying = [&typeManager](const Struct::Field& f)
    {
        // Get the information of the field's type.
        auto typeInfo = typeManager.GetEnum(f.Type);
        switch (typeInfo.UnderlyingSize)
        {
            case 1: return Fundamental::E::UInt8;
            case 2: return Fundamental::E::UInt16;
            case 4: return Fundamental::E::UInt32;
            default: throw std::runtime_error("Bad underlying size for enum!");
        }
    };
    type_id_t underlying_type = static_cast<type_id_t>(to_underlying(field));
    ParseFundamental(table, Struct::Field {.Name = field.Name, .Type = underlying_type}, output);
}

void ParseContainer(const sol::table&     table,
                    const Type::Manager&  typeManager,
                    const Struct::Field&  field,
                    std::vector<uint8_t>& output)
{
    if (typeManager.IsFundamental(field.Type)) { ParseFundamentalContainer(table, field, output); }
    else if (typeManager.IsEnum(field.Type)) { ParseEnumContainer(table, typeManager, field, output); }
    else
    {
        ParseTableContainer(table.get<std::vector<sol::table>>(field.Name),
                            typeManager,
                            typeManager.GetStruct(field.Type).Fields,
                            output);
    }
}

void ParseFundamentalContainer(const sol::table& table, const Struct::Field& field, std::vector<uint8_t>& output)
{
    auto f = [&]<typename T>()
    {
        std::vector<T> v     = table.get<std::vector<T>>(field.Name);
        auto           input = Serialize<std::vector<T>>(v);
        output.insert(output.end(), input.begin(), input.end());
    };
    using Type::Fundamental;
    switch (static_cast<Type::Fundamental::E>(field.Type))
    {
        case Type::Fundamental::E::Bool: f.operator()<bool>(); break;
        case Type::Fundamental::E::Int8: f.operator()<int8_t>(); break;
        case Type::Fundamental::E::UInt8: f.operator()<uint8_t>(); break;
        case Type::Fundamental::E::Int16: f.operator()<int16_t>(); break;
        case Type::Fundamental::E::UInt16: f.operator()<uint16_t>(); break;
        case Type::Fundamental::E::Int32: f.operator()<int32_t>(); break;
        case Type::Fundamental::E::UInt32: f.operator()<uint32_t>(); break;
        case Type::Fundamental::E::Int64: f.operator()<int64_t>(); break;
        case Type::Fundamental::E::UInt64: f.operator()<uint64_t>(); break;
        case Type::Fundamental::E::Float: f.operator()<float>(); break;
        case Type::Fundamental::E::Double: f.operator()<double>(); break;
        case Type::Fundamental::E::String: f.operator()<std::string>(); break;
        case Type::Fundamental::E::Void:
        case Type::Fundamental::E::Size:
        default: BR_LOG_ERROR(s_tag.data(), "Type is not a fundamental"); throw std::exception();
    }
}

void ParseEnumContainer(const sol::table&     table,
                        const Type::Manager&  typeManager,
                        const Struct::Field&  field,
                        std::vector<uint8_t>& output)
{
    using Type::Fundamental;
    auto to_underlying = [&typeManager](const Struct::Field& f)
    {
        // Get the information of the field's type.
        auto typeInfo = typeManager.GetEnum(f.Type);
        switch (typeInfo.UnderlyingSize)
        {
            case 1: return Fundamental::E::UInt8;
            case 2: return Fundamental::E::UInt16;
            case 4: return Fundamental::E::UInt32;
            default: throw std::runtime_error("Bad underlying size for enum!");
        }
    };
    type_id_t underlying_type = static_cast<type_id_t>(to_underlying(field));
    ParseFundamentalContainer(table, Struct::Field {.Name = field.Name, .Type = underlying_type}, output);
}

void ParseTableContainer(const std::vector<sol::table>&    tables,
                         const Type::Manager&              typeManager,
                         const std::vector<Struct::Field>& fields,
                         std::vector<uint8_t>&             output)
{
    auto input = Serialize<serializable_container_size_t>(tables.size());
    output.insert(output.end(), input.begin(), input.end());
    for (const auto& table : tables) { ParseTable(table, typeManager, fields, output); }
}
}    // namespace Frasy::Lua
