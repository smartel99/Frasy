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

#include "event.h"

#include <concepts>
#include <type_traits>

namespace Frasy::Commands
{
template<typename T>
concept CommandHandler = requires(Frasy::Commands::CommandEvent e) {
                             {
                                 T::ExecuteCommand(e)
                             } -> std::same_as<void>;

                             {
                                 T::DestinedTo(e)
                             } -> std::same_as<bool>;
                         };

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
    explicit CommandDispatcher(const Commands::CommandEvent& event, CommandManager& manager) noexcept;

    Commands::CommandEvent m_event;
    CommandManager&        m_manager;
    friend class CommandManager;

    static constexpr const char* s_label = "Dispatcher";
};

class CommandManager
{
    struct CommandEventHandler
    {
        using CommandEvent  = Commands::CommandEvent;
        using destined_to_t = bool (*)(const CommandEvent&);
        using handle_t      = void (*)(const CommandEvent&);

        constexpr CommandEventHandler() = default;
        constexpr CommandEventHandler(destined_to_t destinedTo, handle_t handle, bool enabled)
        : DestinedTo(destinedTo), Handle(handle), IsActive(enabled)
        {
        }

        bool (*DestinedTo)(const CommandEvent&) = nullptr;
        void (*Handle)(const CommandEvent&)     = nullptr;

        bool IsActive = true;
    };

public:
    static CommandManager& Get()
    {
        static CommandManager s_manager;
        return s_manager;
    }

    template<CommandHandler T>
    void AddHandler(bool enabled)
    {
        m_handlers[typeid(T).hash_code()] = MakeHandler<T>(enabled);
    }

    template<CommandHandler T>
    void SetHandlerActiveState(bool active)
    {
        m_handlers.at(typeid(T).hash_code()).IsActive = active;
    }

    template<CommandHandler T>
    void RemoveHandler()
    {
        m_handlers.erase(typeid(T).hash_code());
    }

    CommandDispatcher MakeDispatcher(const Commands::CommandEvent& event) { return CommandDispatcher(event, *this); }

private:
    template<CommandHandler T>
    static CommandEventHandler MakeHandler(bool enabled)
    {
        return CommandEventHandler {T::DestinedTo, T::ExecuteCommand, enabled};
    }

    using HandlerList = std::unordered_map<size_t, CommandEventHandler>;
    static HandlerList MakeHandlerList() { return {}; }


    CommandManager() : m_handlers(MakeHandlerList()) {}

    HandlerList m_handlers = {};

    friend class CommandDispatcher;
};
}    // namespace Frasy::Commands
#endif    // GUARD_INTERFACES_COMMANDS_DISPATCHER_H
