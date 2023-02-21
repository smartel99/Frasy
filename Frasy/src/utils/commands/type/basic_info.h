/**
 * @file    common.h
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
#ifndef INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_BASIC_INFO_H
#define INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_BASIC_INFO_H

#include "fundamental.h"
#include "type_id_t.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace Frasy::Type
{

struct BasicInfo
{
    type_id_t   id;
    std::string name;

    explicit operator std::string() const { return "{" + std::to_string(id) + ", " + std::string(name) + "}"; }

    struct Manager;
};

struct BasicInfo::Manager
{
    static constexpr std::string_view name        = "TypeBasicInfo";
    static constexpr std::string_view description = "Provide essential information about a type";
    static type_id_t                  id;

    // fields
    struct Id
    {
        static constexpr std::string_view name        = "ID";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::UInt16);
        static constexpr std::string_view description = "ID of the type";
    };
    struct Name
    {
        static constexpr std::string_view name        = "Name";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
        static constexpr std::string_view description = "Name of the type";
    };
};
}    // namespace Frasy::Type

#endif    // INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_BASIC_INFO_H
