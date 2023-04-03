/**
 * @file    table_serializer.h
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
#ifndef BRIGERAD_FRASY_SRC_UTILS_LUA_TABLE_SERIALIZER_H
#define BRIGERAD_FRASY_SRC_UTILS_LUA_TABLE_SERIALIZER_H

#include "utils/commands/description/value.h"
#include "utils/commands/type/manager/manager.h"
#include "utils/commands/type/struct.h"

#include <sol/sol.hpp>


namespace Frasy::Lua
{
using Type::Struct;

/// Convert an ordered set of arguments to a table based on a known type
/// Used to normalize Lua function before calling ParseTable when communicating with the instrumentation board
/// @param table the table to build on [OUTPUT]
/// @param typeManager the type manager related to the instrumentation board we converse with
/// @param fields represent the members of the struct
/// @param args values of the fields, mut be provided in the same order as fields
void ArgsToTable(sol::table&                       table,
                 const Type::Manager&              typeManager,
                 const std::vector<Struct::Field>& fields,
                 const sol::variadic_args&         args);

/// Serialize a table to a byte array
/// Used to populate a message payload
/// @param table the table to serialize
/// @param typeManager the type manager related to the instrumentation board we converse with
/// @param fields the members of the struct
/// @param output byte array representing the serialized table
void ParseTable(const sol::table&                 table,
                const Type::Manager&              typeManager,
                const std::vector<Struct::Field>& fields,
                std::vector<uint8_t>&             output);

/// Helper for ParseTable.
/// Parse a fundamental from a table, serialize it and add it to the output array
/// @param table the table to serialize
/// @param field information about the fundamental
/// @param output byte array representing the serialized table
void ParseFundamental(const sol::table& table, const Struct::Field& field, std::vector<uint8_t>& output);

/// Helper for ParseTable.
/// Parse an enum, serialize it and add it to the output array
/// @param table the table to serialize
/// @param typeManager the type manager related to the instrumentation board we converse with
/// @param field information about the enum
/// @param output byte array representing the serialized table
void ParseEnum(const sol::table&     table,
               const Type::Manager&  typeManager,
               const Struct::Field&  field,
               std::vector<uint8_t>& output);

/// Helper for ParseTable.
/// Parse a container field, serialize it and add it to the output array
/// @param table the table to serialize
/// @param typeManager the type manager related to the instrumentation board we converse with
/// @param field information about the container
/// @param output byte array representing the serialized table
void ParseContainer(const sol::table&     table,
                    const Type::Manager&  typeManager,
                    const Struct::Field&  field,
                    std::vector<uint8_t>& output);

/// Helper for ParseContainer.
/// Parse a container of fundamentals, serialize it and add it to the output array
/// @param table the table to serialize
/// @param field information about the fundamental
/// @param output byte array representing the serialized table
void ParseFundamentalContainer(const sol::table& table, const Struct::Field& field, std::vector<uint8_t>& output);

/// Helper for ParseContainer.
/// Parse a container of enum, serialize it and add it to the output array
/// @param table the table to serialize
/// @param typeManager the type manager related to the instrumentation board we converse with
/// @param field information about the enum
/// @param output byte array representing the serialized table
void ParseEnumContainer(const sol::table&     table,
                        const Type::Manager&  typeManager,
                        const Struct::Field&  field,
                        std::vector<uint8_t>& output);


/// Helper for ParseContainer.
/// Parse a container of table, serialize it and add it to the output array
/// @param table the table to serialize
/// @param typeManager the type manager related to the instrumentation board we converse with
/// @param field information about the table
/// @param output byte array representing the serialized table
void ParseTableContainer(const std::vector<sol::table>&    tables,
                         const Type::Manager&              typeManager,
                         const std::vector<Struct::Field>& fields,
                         std::vector<uint8_t>&             output);
}    // namespace Frasy::Lua


#endif    // BRIGERAD_FRASY_SRC_UTILS_LUA_TABLE_SERIALIZER_H
