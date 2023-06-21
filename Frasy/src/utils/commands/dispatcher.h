/**
 * @file    dispatcher.h
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

#ifndef GUARD_INTERFACES_COMMANDS_DISPATCHER_H
#define GUARD_INTERFACES_COMMANDS_DISPATCHER_H

#include "description/command.h"
#include "event.h"

#include <concepts>
#include <type_traits>

namespace Frasy::Commands
{
class CommandManager;


class CommandDispatcher
{
public:
    /**
     * Dispatches the interpreted command to the concerned modules.
     */
    void Dispatch() const noexcept;

private:
    /**
     * Parses and interprets the received packet as a command from the ESP.
     * @param pkt
     */
    explicit CommandDispatcher(Commands::CommandEvent event, CommandManager& manager) noexcept;

    Commands::CommandEvent m_event;
    CommandManager&        m_manager;
    friend class CommandManager;

    static constexpr const char* s_label = "Dispatcher";
};

class CommandManager
{
public:
    static CommandManager& Get()
    {
        static CommandManager s_manager;
        return s_manager;
    }

    void AddCommand(const Actions::Command& command);

    void SetCommandActiveState(const std::string& name, bool active);

    void RemoveHandler(const std::string& name);

    CommandDispatcher MakeDispatcher(const Commands::CommandEvent& event) { return CommandDispatcher(event, *this); }

    size_t                        GetActiveHandlerCount() const noexcept { return m_commands.size(); }
    Actions::Command&             GetCommand(const std::string_view& name) { return m_commands.at(name); }
    std::vector<std::string_view> GetCommandsKeys() const;

private:
    using HandlerList = std::unordered_map<std::string_view, Actions::Command>;
    void MakeHandlerList();

    CommandManager() { MakeHandlerList(); }

    HandlerList m_commands = {};

    friend class CommandDispatcher;
};
}    // namespace Frasy::Commands
#endif    // GUARD_INTERFACES_COMMANDS_DISPATCHER_H
