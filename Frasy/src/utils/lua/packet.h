/**
 * @file    packet.h
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
#ifndef COPY_LUA_PY_FRASY_SRC_UTILS_LUA_PACKET_H
#define COPY_LUA_PY_FRASY_SRC_UTILS_LUA_PACKET_H

#include "../commands/description/command.h"
#include "../communication/serial/device.h"
#include "../communication/serial/packet.h"
#include "sol/sol.hpp"

namespace Frasy::Lua
{
Serial::Packet ToPacket(const Actions::Command& command, const sol::variadic_args& args);

sol::table FromPacket(sol::state& lua, const Serial::Device& device, const Serial::Packet& packet);
}    // namespace Frasy::Lua

#endif    // COPY_LUA_PY_FRASY_SRC_UTILS_LUA_PACKET_H
