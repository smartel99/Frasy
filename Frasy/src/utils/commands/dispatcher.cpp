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

#include "Brigerad.h"

#include "../misc/type_name.h"
#include "utils/commands/built_in/log/command.h"

#include <utility>


namespace Frasy::Commands
{

CommandDispatcher::CommandDispatcher(CommandEvent event, CommandManager& manager) noexcept
: m_event(std::move(event)), m_manager(manager)
{
}

void CommandDispatcher::Dispatch() const noexcept
{
    BR_APP_DEBUG("Looking for handler...");
    for (auto&& [id, command] : m_manager.m_commands)
    {
        if (command.IsForMe(m_event))
        {
            BR_APP_DEBUG("Dispatching command to {}", command.Name.c_str());
            command.Execute(m_event);
        }
    }
    BR_APP_DEBUG("Dispatch complete!");
}

void CommandManager::MakeHandlerList()
{
    AddCommand(Frasy::Actions::Log::Make());
}

void CommandManager::AddCommand(const Actions::Command& command)
{
    m_commands[command.Name] = command;
}

void CommandManager::SetCommandActiveState(const std::string& name, bool active)
{
    m_commands.at(name).Enabled = active;
}

void CommandManager::RemoveHandler(const std::string& name)
{
    m_commands.erase(name);
}

std::vector<std::string_view> CommandManager::GetCommandsKeys() const
{
    std::vector<std::string_view> commands;
    commands.reserve(m_commands.size());
    for (const auto& [key, value] : m_commands) { commands.push_back(key); }
    return commands;
}

}    // namespace Frasy::Commands