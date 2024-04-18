/**
 * @file    command.h
 * @author  Samuel Martel
 * @date    2023-01-05
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

#ifndef FRASY_INTERFACES_COMMANDS_DESCRIPTION_COMMAND_H
#define FRASY_INTERFACES_COMMANDS_DESCRIPTION_COMMAND_H

#include "../event.h"
#include "value.h"

#include <string>
#include <utility>
#include <vector>

namespace Frasy::Actions
{
/**
 * Structure describing a command supported by an instrumentation board.
 */

using Serial::cmd_id_t;
class Command
{
public:
    cmd_id_t                                           Id    = 0;     //!< ID of the command.
    std::string                                        Name  = {};    //!< Name of the command.
    std::string                                        Help  = {};    //!< Help message associated with the command.
    std::string                                        Alias = {};    //!< Supported alias.
    std::vector<Value>                                 Parameters = {};    //!< Parameters taken by the command.
    std::vector<Value>                                 Returned   = {};    //!< Values returned by the command.
    std::function<void(const Commands::CommandEvent&)> Executor   = [](auto) {};

    bool Enabled = true;

    [[nodiscard]] bool IsForMe(const Commands::CommandEvent& e) const { return Id == e.Pkt.Header.CommandId; }
    void               Execute(const Commands::CommandEvent& e) { return Executor(e); }


    explicit operator std::string() const
    {
        return "{" + std::to_string(Id) + ", " +      //
               std::string(Name) + ", " +             //
               std::string(Help) + ", " +             //
               std::string(Alias) + ", " +            //
               VectorToString(Parameters) + ", " +    //
               VectorToString(Returned) + "}";
    }
};
}    // namespace Frasy::Actions

#endif    // FRASY_INTERFACES_COMMANDS_DESCRIPTION_COMMAND_H
