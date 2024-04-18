/**
 * @file    id.h
 * @author  Paul Thomas
 * @date    2023-02-07
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
#ifndef INTERFACE_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_ID_H
#define INTERFACE_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_ID_H
#include "../../communication/serial/types.h"

#include <cstdint>

namespace Frasy::Actions
{
using Frasy::Serial::cmd_id_t;

enum class CommandId : cmd_id_t
{
    Identify = 0x8000,
    Status,
    Reset,
    Log,
    CommandsList,
    CommandInfo,
    EnumsList,
    EnumInfo,
    StructsList,
    StructInfo,
};
}    // namespace Frasy::Actions

#endif    // INTERFACE_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_ID_H
