/**
 * @file    populate_map.cpp
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

#include "orchestrator.h"

namespace Frasy::Lua
{

void Orchestrator::PopulateMap()
{
    m_map = {};

    m_map.count.uut   = (*m_state)["Context"]["Map"]["count"]["uut"].get<std::size_t>();
    m_map.count.ib    = (*m_state)["Context"]["Map"]["count"]["ib"].get<std::size_t>();
    m_map.count.teams = (*m_state)["Context"]["Team"]["teams"].get<std::vector<sol::object>>().size();

    if (m_map.count.teams != 0)
    {
        for (auto& [k, v] : (*m_state)["Context"]["Team"]["teams"].get<sol::table>())
        {
            std::size_t leader               = k.as<std::size_t>();
            std::size_t ib                   = (*m_state)["Context"]["Map"]["uut"][leader]["ib"].get<std::size_t>();
            m_map.ibs[ib].teams[leader].uuts = v.as<std::vector<std::size_t>>();
        }
    }
    else
    {
        for (auto& [ib, ibt] : (*m_state)["Context"]["Map"]["ib"].get<sol::table>())
        {
            for (auto& [_, uut] : ibt.as<sol::table>()["uut"].get<sol::table>())
            {
                m_map.ibs[ib.as<std::size_t>()].teams[uut.as<std::size_t>()].uuts = {uut.as<std::size_t>()};
            }
        }
    }

    for (std::size_t i = 1; i <= m_map.count.uut; ++i) { m_map.uuts.push_back(i); }
}

}    // namespace Frasy::Lua
