/**
 * @file    version.cpp
 * @author  Paul Thomas
 * @date    5/16/2024
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

#include "version.h"
#include <regex>

Version Version::parse(const std::string& version)
{
    Version     v {};
    std::regex  re("^(\\d+)(?:\\.(\\d+))?(?:\\.(\\d+))?(?:\\.(\\d+))?$");
    std::smatch m;
    if (!std::regex_match(version, m, re)) { return v; }
    v.major = std::stoi(m[1].str());
    if (!m[2].matched) { return v; }
    v.minor = std::stoi(m[2].str());
    if (!m[3].matched) { return v; }
    v.revision = std::stoi(m[3].str());
    if (!m[4].matched) { return v; }
    v.build = std::stoi(m[4].str());
    return v;
}