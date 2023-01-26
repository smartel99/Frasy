/**
 * @file    dispatcher.cpp
 * @author  Samuel Martel
 * @date    2022-09-21
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
#include "dispatcher.h"

#include <Brigerad/Core/Log.h>


namespace Frasy::Commands
{
CommandDispatcher::CommandDispatcher(const CommandEvent& event, CommandManager& manager) noexcept
: m_event(event), m_manager(manager)
{
}

void CommandDispatcher::Dispatch() const noexcept
{
    BR_APP_DEBUG("Dispatching command!");
    for (auto&& [t, handler] : m_manager.m_handlers)
    {
        if (handler.DestinedTo(m_event)) { handler.Handle(m_event); }
    }
}
}    // namespace Frasy::Commands
