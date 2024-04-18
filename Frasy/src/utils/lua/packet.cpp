/**
 * @file    packet.cpp
 * @author  Paul Thomas
 * @date    2023-03-06
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

#include "packet.h"

#include "tag.h"

namespace Frasy::Lua
{
Serial::Packet ToPacket(const Actions::Command& command, const sol::variadic_args& args)
{
    Serial::Packet packet;
    if (command.Parameters.size() != args.size())
    {
        BR_LOG_ERROR(
          s_tag.data(), "Invalid number of arguments. expected {}, got {}", command.Parameters.size(), args.size());
        throw std::exception();
    }

    std::vector<Type::Struct::Field> fields;
    std::transform(command.Parameters.begin(),
                   command.Parameters.end(),
                   fields.begin(),
                   [](const Actions::Value& value)
                   {
                       return Type::Struct::Field {
                         .Name  = value.Name,
                         .Type  = value.Type,
                         .Count = value.Count,
                       };
                   });

    sol::state_view lua   = sol::state_view(args.lua_state());
    sol::table      table = lua.create_table();

    //    ArgsToTable(table, fields, args);
    //    ParseTable(table, fields, packet.Payload);

    packet.UpdatePayloadSize();
    return packet;
}


sol::table FromPacket(sol::state& lua, const Serial::Device& device, const Serial::Packet& packet)
{
    sol::table table = lua.create_table();
    table["id"]      = packet.Header.CommandId;
    table["payload"] = lua.create_table();

//    if (!device.getCommands().contains(packet.Header.CommandId))
//    {
//        BR_LOG_ERROR(s_tag.data(), "Unknown command {}", packet.Header.CommandId);
//    }
//    else { const auto& command = device.getCommands().at(static_cast<Actions::cmd_id_t>(packet.Header.CommandId)); }
    return table;
}
}    // namespace Frasy::Lua
