/**
 * @file    fundamental.h
 * @author  Paul Thomas
 * @date    2023-02-14
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
#ifndef INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_FUNDAMENTAL_H
#define INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_FUNDAMENTAL_H

#include "type_id_t.h"

#include <string_view>

namespace Frasy::Type
{
struct Fundamental
{
    enum class E : type_id_t
    {
        Void,
        Bool,
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
        String,
        Size
    };

    struct Manager
    {
        static constexpr std::string_view name = "Fundamental";
        static type_id_t                  id;
    };

    static inline constexpr std::string_view ToStr(E e)
    {
        switch(e)
        {
            case E::Void: return "void";
            case E::Bool: return "bool";
            case E::Int8: return "int8";
            case E::UInt8: return "uint8";
            case E::Int16: return "int16";
            case E::UInt16: return "uint16";
            case E::Int32: return "int32";
            case E::UInt32: return "uint32";
            case E::Int64: return "int64";
            case E::UInt64: return "uint64";
            case E::Float: return "float";
            case E::Double: return "double";
            case E::String: return "string";
            case E::Size: return "size";
            default:
                return "unknown";
        }
    }
};
}    // namespace Frasy::Type

#endif    // INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_FUNDAMENTAL_H
