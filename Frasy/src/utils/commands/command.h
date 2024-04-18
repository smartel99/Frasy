/**
 * @file    command.h
 * @author  Samuel Martel
 * @date    2022-09-22
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

#ifndef GUARD_NILAI_INTERFACES_COMMANDS_COMMAND_H
#define GUARD_NILAI_INTERFACES_COMMANDS_COMMAND_H
#include "Brigerad/Core/Core.h"
#include "payload.h"
#include "response.h"

#include <concepts>
#include <cstdint>
#include <type_traits>

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
template<typename T>
concept ValidCommandId = requires { T::id; } && std::same_as<std::remove_cvref_t<decltype(T::id)>, uint16_t>;

/**
 * For a command to be valid, it must be composed in a certain way.
 *
 * <ul>
 *  <li>
 *      It must have a static constexpr member called `id` of the type `uint8_t`. This is the
 *      command's ID.
 *  </li>
 *  <li>
 *      It must have a typename called `payload_type`. This is the type of the `payload`. It may be
 *      `void`, primitive, `std::vector<uint8_t>`, `std::string` or any other object that is
 *      constructable from an `std::array` of `uint8_t`.
 *  </li>
 *  <li>
 *     If `payload_type` is not void, a static constexpr member called `payload_size` of type
 *     size_t must be present. This corresponds to the size of the payload.
 *  </li>
 *  <li>
 *      If `payload_type` is not void, a member of type `payload_type` called `payload` must be
 *      present. This payload must be implicitly/explicitly convertible to
 *      `std::vector<uint8_t>`, or respecting the `Primitives` concept.
 *  </li>
 *  <li>
 *      It must have a typename called `response_type`. This it the type of the response expected
 *      to that command. It may be `void`, a primitive, `std::vector<uint8_t>`, `std::string` or
 *      any other object that is constructable from a `std::vector<uint8_t>`.
 *  </li>
 * </ul>
 *
 *
 * @tparam T
 */
template<typename T>
concept Command =
  ValidCommandPayload<T> && ValidCommandResponse<T> && ValidCommandId<T> && std::is_default_constructible_v<T>;


template<uint16_t        Id,
         CommandPayload  Payload      = void,
         CommandResponse Response     = void,
         int             PayloadSize  = -1,
         int             ResponseSize = -1>
struct GenericCommand
{
    using payload_type = Payload;
    // If PayloadSize is not provided, automatically deduce the size of the payload. Otherwise,
    // use the provided payload size.
    static constexpr size_t payload_size = PayloadSize == -1 ? CommandPayloadSize<Payload>::value : PayloadSize;

    using response_type                   = Response;
    static constexpr size_t response_size = ResponseSize == -1 ? CommandResponseSize<Response>::value : ResponseSize;

    static constexpr uint16_t id = Id;

    template<typename... Ts>
    static auto MakeCommand(Ts&&... ts)
        requires(std::same_as<Ts, payload_type> && ...)
    {
        auto impl = [](const std::vector<uint8_t>& data) { return Serial::Packet(id, data, false, true); };
        static_assert(sizeof...(Ts) <= 1, "Only one payload allowed!");
        if constexpr (sizeof...(Ts) == 0) { return impl({}); }
        else { return impl(Serialize(ts...)); }
    }


    static bool IsPacketForMe(const Serial::Packet& pkt) { return id == pkt.Header.CommandId; }

    static response_type GetResponse(const Serial::Packet& pkt)
    {
        if constexpr (std::same_as<response_type, void>) { return; }
        else
        {
            BR_ASSERT(pkt.Header.PayloadSize == response_size,
                      "Not enough data for response ({}/{})!",
                      pkt.Header.PayloadSize,
                      response_size);
            return Frasy::Deserialize<response_type>(pkt.Header);
        }
    }

private:
};
}    // namespace Frasy::Commands

#endif    // GUARD_NILAI_INTERFACES_COMMANDS_COMMAND_H
