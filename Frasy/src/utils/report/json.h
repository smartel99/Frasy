/**
 * @file    json.h
 * @author  Paul Thomas
 * @date    3/17/2025
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

#ifndef FRASY_SRC_UTILS_REPORT_JSON_H
#define FRASY_SRC_UTILS_REPORT_JSON_H

#include <vector>
#include <string>
#include <sol/sol.hpp>

namespace Frasy::Report::Json {
std::vector<std::string> makeReport(sol::state_view&                lua,
                                    const sol::table&               results,
                                    const std::vector<std::string>& filenames);
}

#endif //FRASY_SRC_UTILS_REPORT_FORMATTER_JSON_H
