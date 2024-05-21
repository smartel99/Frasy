/**
 * @file    version.h
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
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */
#ifndef FRASY_SRC_UTILS_LUA_VERSION_H
#define FRASY_SRC_UTILS_LUA_VERSION_H

#include "utils/models/version.h"
#include <sol/sol.hpp>

Version versionFromLuaTable(sol::table table)
{
    return {
      table["major"].get_or<std::size_t>(0),
      table["minor"].get_or<std::size_t>(0),
      table["revision"].get_or<std::size_t>(0),
      table["build"].get_or<std::size_t>(0),
    };
}

sol::table versionToLuaTable(sol::state_view lua, const Version& version)
{
    auto table        = lua.create_table();
    table["major"]    = version.major;
    table["minor"]    = version.minor;
    table["revision"] = version.revision;
    table["build"]    = version.build;
    return table;
}

#endif    // FRASY_SRC_UTILS_LUA_VERSION_H
