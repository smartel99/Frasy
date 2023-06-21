/**
 * @file    event.h
 * @author  Samuel Martel
 * @date    2022-10-11
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

#ifndef GUARD_NILAI_INTERFACES_COMMANDS_EVENT_H
#define GUARD_NILAI_INTERFACES_COMMANDS_EVENT_H

#include "../communication/serial/packet.h"
#include "command.h"

#include <functional>
#include <optional>
#include <utility>

/**
 * @addtogroup Nilai
 * @{
 */

/**
 * @addtogroup Interfaces
 * @{
 */

/**
 * @addtogroup Commands
 * @{
 */

namespace Frasy::Commands
{
struct CommandEvent
{
    using RespondFunc = std::function<void(const Communication::Packet&)>;
    RespondFunc           Respond;
    std::string           Source;
    Communication::Packet Pkt;

    explicit CommandEvent(RespondFunc f, std::string src, Communication::Packet pkt)
    : Respond(std::move(f)), Source(std::move(src)), Pkt(std::move(pkt))
    {
    }

    template<Command Cmd>
    [[nodiscard]] bool Is() noexcept
    {
        return Pkt.Header.CommandId == Cmd::id && HasEnoughDataForCmd<Cmd>();
    }

    template<Command Cmd>
    [[nodiscard]] std::optional<Cmd> As() noexcept
    {
        if (!Is<Cmd>()) { return std::nullopt; }

        return Cmd {Pkt.Payload};
    }

private:
    template<Command Cmd>
    [[nodiscard]] bool HasEnoughDataForCmd() const noexcept
    {
        size_t expectedSize = 0;
        if constexpr (CommandHasPayload<Cmd>()) { expectedSize += Cmd::payload_size; }

        if (Pkt.Header.PayloadSize >= expectedSize) { return true; }
        else { return false; }
    }
};
}    // namespace Frasy::Commands
//!@}
//!@}
//!@}
#endif    // GUARD_NILAI_INTERFACES_COMMANDS_EVENT_H
