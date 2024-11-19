/**
 * @file    spin_input.h
 * @author  Samuel Martel
 * @date    2024-10-22
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


#ifndef FRASY_SRC_UTILS_IMGUI_SPIN_INPUT_H
#define FRASY_SRC_UTILS_IMGUI_SPIN_INPUT_H

#include <imgui.h>

#include <limits>

namespace Frasy::Widget {
bool spinInt(const char*         label,
             int*                v,
             int                 step      = 1,
             int                 stepFast = 100,
             int                 min       = std::numeric_limits<int>::min(),
             int                 max       = std::numeric_limits<int>::max(),
             const char* format = "%d");
}    // namespace Frasy::Widget

#endif    // FRASY_SRC_UTILS_IMGUI_SPIN_INPUT_H
