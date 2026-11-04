/**
 * @file    ib_info.h
 * @author  Sam Martel
 * @date    2026-05-26
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


#ifndef FRASY_LUA_IB_INFO_H
#define FRASY_LUA_IB_INFO_H

#include <sol/sol.hpp>

namespace Frasy::Lua {
/**
 * To be used by "instrumentation boards" defined in C++.
 */
struct IbInfo {
    int         kind    = 0;
    int         nodeId  = 0;
    std::string name    = "<N/A>";
    std::string edsPath = "<N/A>";
    sol::table  od;    //!< Object dictionary loaded in lua. Can be empty.

    std::string serialNumber    = "<N/A>";
    std::string softwareVersion = "<N/A>";
    std::string hardwareVersion = "<N/A>";

    static void registerLuaType(sol::state_view lua)
    {
        lua.new_usertype<IbInfo>(
          "IbInfo",
          "kind",
          &IbInfo::kind,
          "nodeId",
          &IbInfo::nodeId,
          "name",
          &IbInfo::name,
          "edsPath",
          &IbInfo::edsPath,
          "od",
          &IbInfo::od,
          "Serial",
          [](IbInfo& info) { return info.serialNumber; },
          "SoftwareVersion",
          [](IbInfo& info) { return info.softwareVersion; },
          "HardwareVersion",
          [](IbInfo& info) { return info.hardwareVersion; });
    }
};
}    // namespace Frasy::Lua

#endif    // FRASY_LUA_IB_INFO_H
