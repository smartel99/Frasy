/**
 * @file    types.h
 * @author  Samuel Martel
 * @date    2022-12-13
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

#ifndef FRASY_UTILS_COMMUNICATION_SERIAL_TYPES_H
#define FRASY_UTILS_COMMUNICATION_SERIAL_TYPES_H

#include <cstdint>
#include <limits>

namespace Frasy::Communication
{
using trs_id_t       = std::uint32_t;
using cmd_id_t       = std::uint16_t;
using payload_size_t = std::uint16_t;

inline constexpr trs_id_t AUTOMATIC_TRANSACTION_ID = std::numeric_limits<trs_id_t>::max();
inline constexpr cmd_id_t INVALID_COMMAND_ID       = std::numeric_limits<cmd_id_t>::max();

struct PacketModifiers
{
    bool IsResponse      = false;
    bool PayloadAsString = false;

    PacketModifiers() = default;

    explicit constexpr PacketModifiers(uint8_t v) noexcept
    : IsResponse(((v & s_isRespMask) >> s_isRespShift) != 0),
      PayloadAsString(((v & s_asStringMask) >> s_asStringShift) != 0)
    {
    }
    constexpr PacketModifiers(bool isResp, bool asString) noexcept : IsResponse(isResp), PayloadAsString(asString) {}

    explicit constexpr operator uint8_t() const noexcept
    {
        return (static_cast<uint8_t>(IsResponse) << s_isRespShift) |
               (static_cast<uint8_t>(PayloadAsString) << s_asStringShift);
    }

    [[nodiscard]] bool operator==(const PacketModifiers& other) const
    {
        return IsResponse == other.IsResponse && PayloadAsString == other.PayloadAsString;
    }

private:
    static constexpr uint8_t s_isRespShift = 0x00;
    static constexpr uint8_t s_isRespMask  = 0x01;

    static constexpr uint8_t s_asStringShift = 0x01;
    static constexpr uint8_t s_asStringMask  = 0x02;
};


}    // namespace Frasy::Communication

#endif    // FRASY_UTILS_COMMUNICATION_SERIAL_TYPES_H
