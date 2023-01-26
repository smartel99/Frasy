/**
 * @file    command_value_type.h
 * @author  Samuel Martel
 * @date    2023-01-04
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

#ifndef FRASY_INSTRUMENTATION_CARD_COMMAND_VALUE_TYPE_H
#define FRASY_INSTRUMENTATION_CARD_COMMAND_VALUE_TYPE_H

#include <string>
#include <string_view>
#include <vector>

namespace Frasy::Instrumentation
{
enum ValueType
{
    Bool = 0,
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Float,
    Double,
    String
};

inline constexpr std::string_view ValueTypeStr(ValueType valueType)
{
    using namespace std::string_view_literals;
    switch (valueType)
    {
        case Bool: return "Bool"sv;
        case Int8: return "Int8"sv;
        case UInt8: return "UInt8"sv;
        case Int16: return "Int16"sv;
        case UInt16: return "Uint16"sv;
        case Int32: return "Int32"sv;
        case UInt32: return "UInt32"sv;
        case Int64: return "Int64"sv;
        case UInt64: return "UInt64"sv;
        case Float: return "Float"sv;
        case Double: return "Double"sv;
        case String: return "String"sv;
        default: return "Unknown"sv;
    }
}

inline constexpr size_t ValueTypeSize(ValueType valueType)
{
    switch (valueType)
    {
        case Bool: return sizeof(bool);
        case Int8: return sizeof(int8_t);
        case UInt8: return sizeof(uint8_t);
        case Int16: return sizeof(int16_t);
        case UInt16: return sizeof(uint16_t);
        case Int32: return sizeof(int32_t);
        case UInt32: return sizeof(uint32_t);
        case Int64: return sizeof(int64_t);
        case UInt64: return sizeof(uint64_t);
        case Float: return sizeof(float);
        case Double: return sizeof(double);
        case String: return sizeof(char);
        default: return 0;
    }
}


union ValueRanges
{
    struct
    {
        bool min = false;
        bool max = true;
    } Bool;
    struct
    {
        int8_t min = 0;
        int8_t max = 0;
    } Int8;
    struct
    {
        uint8_t min = 0;
        uint8_t max = 0;
    } UInt8;
    struct
    {
        uint16_t min = 0;
        uint16_t max = 0;
    } Int16;
    struct
    {
        uint16_t min = 0;
        uint16_t max = 0;
    } UInt16;
    struct
    {
        int32_t min = 0;
        int32_t max = 0;
    } Int32;
    struct
    {
        uint32_t min = 0;
        uint32_t max = 0;
    } UInt32;
    struct
    {
        int64_t min = 0;
        int64_t max = 0;
    } Int64;
    struct
    {
        uint64_t min = 0;
        uint64_t max = 0;
    } UInt64;
    struct
    {
        float min = 0;
        float max = 0;
    } Float;
    struct
    {
        double min = 0;
        double max = 0;
    } Double;
    struct
    {
        size_t minLen = 0;
        size_t maxLen = 0;
    } String;
};

}    // namespace Frasy::Instrumentation

#endif    // FRASY_INSTRUMENTATION_CARD_COMMAND_VALUE_TYPE_H
