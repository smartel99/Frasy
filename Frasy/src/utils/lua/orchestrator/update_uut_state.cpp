/**
 * @file    update_uut_state.cpp
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
void Orchestrator::UpdateUutState(enum UutState state, bool force)
{
    std::vector<std::size_t> uuts;
    UpdateUutState(state, m_map.uuts, force);
}

void Orchestrator::UpdateUutState(enum UutState state, const std::vector<std::size_t>& uuts, bool force)
{
    for (auto uut : uuts)
    {
        if (m_uutStates[uut] == UutState::Disabled && !force) continue;
        m_uutStates[uut] = state;
    }
}
}    // namespace Frasy::Lua