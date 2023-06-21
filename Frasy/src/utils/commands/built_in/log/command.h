/**
 * @file    command.h
 * @author  Paul Thomas
 * @date    2023-02-23
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
#ifndef BRIGERAD_FRASY_SRC_UTILS_COMMANDS_BUILT_IN_LOG_COMMAND_H
#define BRIGERAD_FRASY_SRC_UTILS_COMMANDS_BUILT_IN_LOG_COMMAND_H

#include "utils/commands/description/command.h"

namespace Frasy::Actions::Log
{
Command Make();
}

#endif    // BRIGERAD_FRASY_SRC_UTILS_COMMANDS_BUILT_IN_LOG_COMMAND_H
