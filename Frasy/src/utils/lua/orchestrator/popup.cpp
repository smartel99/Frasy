/**
 * @file    popup.cpp
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

#include "utils/lua/popup.h"

#include "orchestrator.h"

#include <imgui.h>

namespace Frasy::Lua
{
void Orchestrator::RenderPopups()
{
    std::lock_guard lock {(*m_popupMutex)};
    for (auto& [name, popup] : m_popups) popup->Render();
}

void Orchestrator::ImportPopup(sol::state& lua, std::size_t uut)
{
    lua.script_file("lua/core/sdk/popup.lua");
    lua["__popup"]            = lua.create_table();
    lua["__popup"]["consume"] = [&, uut](sol::table builder) { m_popups[Popup::GetName(uut, builder)]->Consume(); };
    lua["__popup"]["show"]    = [&, uut](sol::table builder)
    {
        Popup popup = Popup(uut, builder);
        m_popupMutex->lock();
        m_popups[popup.GetName()] = &popup;
        m_popupMutex->unlock();
        popup.Routine();
        m_popupMutex->lock();
        m_popups.erase(popup.GetName());
        m_popupMutex->unlock();
        return popup.GetInputs();
    };
}
}    // namespace Frasy::Lua