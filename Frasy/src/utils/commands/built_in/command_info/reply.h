/**
 * @file    command_info.h
 * @author  Paul Thomas
 * @date    2023-02-07
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
#ifndef INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_COMMAND_INFO_REPLY_H
#define INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_COMMAND_INFO_REPLY_H

#include "../../description/command.h"
#include "../../description/value.h"
#include "../id.h"
#include "Brigerad/Core/Core.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Frasy::Actions::CommandInfo
{
using Frasy::Type::Fundamental;
struct Reply
{
    cmd_id_t           Id = 0;
    std::string        Name;
    std::string        Help;
    std::string        Alias;
    std::vector<Value> Parameters;
    std::vector<Value> Returns;

    explicit operator std::string() const
    {
        return "{" + std::string(Name) + ", " + std::string(Help) + ", " + std::to_string(Id) + ", " +
               VectorToString(Parameters) + ", " + VectorToString(Returns) + ", " + std::string(Alias) + "}";
    }

    struct Manager;
};


struct Reply::Manager
{
    static cmd_id_t                   id;
    static constexpr std::string_view name        = "CommandInfo::Reply";
    static constexpr std::string_view description = "Reply from Command Info";

    // fields
    struct Name
    {
        static constexpr std::string_view name        = "Name";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
        static constexpr std::string_view description = "Name of the command. Can be used to invoke it";
    };
    struct Help
    {
        static constexpr std::string_view name        = "Help";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
        static constexpr std::string_view description = "Usage of the command";
    };
    struct Id
    {
        static constexpr std::string_view name        = "Id";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::UInt16);
        static constexpr std::string_view description = "Unique Id for this command";
    };
    struct Parameters
    {
        static constexpr std::string_view name = "Parameters";
        static type_id_t                  type()
        {
            BR_ASSERT(Frasy::Actions::Value::Manager::id != 0, "Invalid ID");
            return Frasy::Actions::Value::Manager::id;
        };
        static constexpr std::string_view description = "Parameters required for this command";
    };
    struct Returns
    {
        static constexpr std::string_view name = "Returns";
        static type_id_t                  type()
        {
            BR_ASSERT(Frasy::Actions::Value::Manager::id != 0, "Invalid ID");
            return Frasy::Actions::Value::Manager::id;
        };
        static constexpr std::string_view description = "Values returned by the command";
    };
    struct Alias
    {
        static constexpr std::string_view name        = "Alias";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
        static constexpr std::string_view description = "Shorter command name. Can be used to invoke it.";
    };
};

}    // namespace Frasy::Actions::CommandInfo
#endif    // INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_COMMAND_INFO_REPLY_H
