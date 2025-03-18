/**
 * @file    obj2str.h
 * @author  Paul Thomas
 * @date    3/18/2025
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
 * not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FRASY_SRC_UTILS_REPORT_UTILS_OBJ2STR_H
#define FRASY_SRC_UTILS_REPORT_UTILS_OBJ2STR_H

#include <sol/sol.hpp>
#include <string>
#include <spdlog/spdlog.h>

namespace Frasy::Report {
/**
* Convert a lua object to its string representation
*
* Note: this is NOT a recursive function, it will not iterate through tables
*/
auto obj2str(const sol::object& object) -> std::string;
}

#endif //FRASY_SRC_UTILS_REPORT_UTILS_OBJ2STR_H
