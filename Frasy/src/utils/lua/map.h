/**
 * @file    map.h
 * @author  Paul Thomas
 * @date    3/28/2023
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
#ifndef COPY_LUA_PY_FRASY_SRC_UTILS_LUA_MAP_H
#define COPY_LUA_PY_FRASY_SRC_UTILS_LUA_MAP_H

#include "ib_view.h"

#include <map>
#include <vector>

namespace Frasy::Lua {
struct Map {
    std::map<std::string, IbView>         ibs;
    std::vector<std::size_t>              uuts;
    std::vector<std::vector<std::size_t>> teams;
};
}    // namespace Frasy::Lua

#endif    // COPY_LUA_PY_FRASY_SRC_UTILS_LUA_MAP_H
