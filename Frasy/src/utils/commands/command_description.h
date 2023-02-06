/**
 * @file    command_description.h
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

#ifndef FRASY_INSTRUMENTATION_CARD_COMMAND_DESCRIPTION_H
#define FRASY_INSTRUMENTATION_CARD_COMMAND_DESCRIPTION_H

#include "command_value_type.h"

#include <array>
#include <string>
#include <vector>

namespace Frasy::Commands
{
/**
 * Structure describing a command's value.
 * This value can either be part of the argument list taken by the command,
 * or it can be part of the response given by that command.
 */
struct CommandValue
{
    uint16_t    Id = 0;        //!< Positional ID of the value.
    std::string Name;          //!< Name associated to the value.
    ValueType   Type;          //!< Type of the value.
    uint8_t     Count = 0;     //!< Number of values for this value (values other than 1 makes the value an array).
    ValueRanges Range = {};    //!< Allowed range that the value can take.
    std::string Help;          //!< Help message associated with the string.
};

/**
 * Structure describing a command supported by an instrumentation card.
 */
struct CommandDescription
{
    uint16_t    Id = 0;                    //!< ID of the command.
    std::string Name;                      //!< Name of the command.
    std::vector<CommandValue> Parameters;  //!< Parameters taken by the command.
    std::vector<CommandValue> Returned;    //!< Values returned by the command.
    std::string Help;                      //!< Help message associated with the command.
};
}  // namespace Frasy::Commands

#endif    // FRASY_INSTRUMENTATION_CARD_COMMAND_DESCRIPTION_H
