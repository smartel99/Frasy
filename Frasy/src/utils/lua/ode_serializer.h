/**
 * @file    sdo_serializer.h
 * @author  Paul Thomas
 * @date    5/21/2024
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
#ifndef FRASY_SRC_UTILS_LUA_ODE_SERIALIZER_H
#define FRASY_SRC_UTILS_LUA_ODE_SERIALIZER_H

#include <cstdint>
#include <sol/sol.hpp>
#include <vector>

std::vector<uint8_t> serializeOdeValue(const sol::table& ode, const sol::object& value);

#endif    // FRASY_SRC_UTILS_LUA_ODE_SERIALIZER_H
