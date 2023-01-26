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

namespace Frasy::Communication
{
using pkt_id_t                                = std::uint32_t;
static constexpr pkt_id_t AUTOMATIC_PACKET_ID = std::numeric_limits<pkt_id_t>::max();

using cmd_id_t       = std::uint16_t;
using blk_id_t       = std::uint8_t;
using payload_size_t = std::uint8_t;

struct PacketModifiers
{
    bool IsResponse   = false;
    bool IsLastPacket = false;

    explicit constexpr PacketModifiers(uint8_t v) noexcept
    : IsResponse(((v & s_isRespMask) >> s_isRespShift) != 0), IsLastPacket(((v & s_isLastMask) >> s_isLastShift) != 0)
    {
    }
    constexpr PacketModifiers(bool isResp, bool isLast) noexcept : IsResponse(isResp), IsLastPacket(isLast) {}

    explicit constexpr operator uint8_t() const noexcept
    {
        return (static_cast<uint8_t>(IsResponse) << s_isRespShift) |
               (static_cast<uint8_t>(IsLastPacket) << s_isLastShift);
    }

private:
    static constexpr uint8_t s_isRespShift = 0x00;
    static constexpr uint8_t s_isRespMask  = 0x01;

    static constexpr uint8_t s_isLastShift = 0x01;
    static constexpr uint8_t s_isLastMask  = 0x02;
};


}    // namespace Frasy::Communication

#endif    // FRASY_UTILS_COMMUNICATION_SERIAL_TYPES_H
