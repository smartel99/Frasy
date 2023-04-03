/**
 * @file    log.cpp
 * @author  Paul Thomas
 * @date    4/3/2023
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

#include "Brigerad/Core/Log.h"

#include "orchestrator.h"

namespace Frasy::Lua
{

void Orchestrator::ImportLog(sol::state& lua, std::size_t uut)
{
    lua.script_file("lua/core/sdk/log.lua");
    lua["Log"]["c"] = [uut](std::string message) { BR_LOG_CRITICAL(std::format("UUT{}", uut), message); };
    lua["Log"]["e"] = [uut](std::string message) { BR_LOG_ERROR(std::format("UUT{}", uut), message); };
    lua["Log"]["w"] = [uut](std::string message) { BR_LOG_WARN(std::format("UUT{}", uut), message); };
    lua["Log"]["i"] = [uut](std::string message) { BR_LOG_INFO(std::format("UUT{}", uut), message); };
    lua["Log"]["d"] = [uut](std::string message) { BR_LOG_DEBUG(std::format("UUT{}", uut), message); };
    lua["Log"]["y"] = [uut](std::string message) { BR_LOG_TRACE(std::format("UUT{}", uut), message); };
}

}    // namespace Frasy::Lua