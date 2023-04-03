/**
 * @file    init.cpp
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

void Orchestrator::Init(const std::string& environment, const std::string& testsDir)
{
    m_state       = std::make_unique<sol::state>();
    m_map         = {};
    m_generated   = false;
    m_environment = environment;
    m_testsDir    = testsDir;
    InitLua(*m_state);
    if (!LoadEnvironment(*m_state, m_environment)) return;
    if (!LoadTests(*m_state, m_testsDir)) return;
    PopulateMap();
    m_uutStates.resize(m_map.count.uut + 1, UutState::Idle);
    m_popupMutex = std::make_unique<std::mutex>();
}

}    // namespace Frasy::Lua