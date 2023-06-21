/**
 * @file    args_checker.h
 * @author  Paul Thomas
 * @date    3/27/2023
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
#ifndef COPY_LUA_PY_FRASY_SRC_UTILS_LUA_ARGS_CHECKER_H
#define COPY_LUA_PY_FRASY_SRC_UTILS_LUA_ARGS_CHECKER_H

#include "utils/commands/type/manager/manager.h"
#include "utils/commands/type/struct.h"

#include <sol/sol.hpp>
#include <vector>

namespace Frasy::Lua
{

void CheckArgs(sol::state_view                                lua,
               const Frasy::Type::Manager&                    typeManager,
               const std::vector<Frasy::Type::Struct::Field>& fields,
               sol::variadic_args&                            args);

}    // namespace Frasy::Lua


#endif    // COPY_LUA_PY_FRASY_SRC_UTILS_LUA_ARGS_CHECKER_H
