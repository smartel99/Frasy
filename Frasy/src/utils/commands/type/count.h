/**
 * @file    count.h
 * @author  Paul Thomas
 * @date    2023-02-16
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
#ifndef INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_COUNT_H
#define INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_COUNT_H

#include <cstdint>

namespace Frasy::Type
{
static constexpr uint16_t VECTOR = 0;
static constexpr uint16_t SINGLE = 1;
static constexpr uint16_t ARRAY(uint16_t size) { return size; }
}    // namespace Frasy::Type

#endif    // INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_COUNT_H
