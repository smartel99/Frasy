/**
 * @file    get_commands.h
 * @author  Paul Thomas
 * @date    2023-02-06
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

#ifndef FRASY_SRC_INSTRUMENTATION_CARD_GET_COMMANDS_H
#define FRASY_SRC_INSTRUMENTATION_CARD_GET_COMMANDS_H

#include "utils/commands/command.h"
#include "utils/misc/deserializer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Frasy::Actions::CommandsList
{
using Reply = std::vector<std::string>;
static_assert(std::same_as<decltype(Deserialize<Reply>(nullptr, nullptr)), Reply>);

using CommandInfo = Commands::GenericCommand<0x8001, void, Reply>;
}    // namespace Frasy::Actions::CommandsList

#endif    // FRASY_SRC_INSTRUMENTATION_CARD_GET_COMMANDS_H
