/**
 * @file    sync.cpp
 * @author  Paul Thomas
 * @date    3/30/2023
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

#include "orchestrator.h"

namespace Frasy::Lua
{
void Orchestrator::ImportSync(sol::state& lua)
{
    lua.script_file("lua/core/sdk/sync.lua");
    if (lua["Context"]["stage"].get<Stage>() != Stage::Execution) return;
    lua["Sync"]["Global"] = [&]() { m_globalSync->arrive_and_wait(); };
}
}    // namespace Frasy::Lua
