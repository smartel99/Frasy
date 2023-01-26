/**
 * @file    packet.cpp
 * @author  Samuel Martel
 * @date    2022-12-08
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

#include "../../misc/char_conv.h"
#include "exceptions.h"

#include <Brigerad/Core/Log.h>

namespace Frasy::Communication
{
constexpr PacketHeader::PacketHeader(
  pkt_id_t pktId, cmd_id_t cmdId, PacketModifiers mods, blk_id_t blkId, payload_size_t payloadSize)
: PacketId(pktId == AUTOMATIC_PACKET_ID ? s_lastPktId : pktId),
  CommandId(cmdId),
  Modifiers(mods),
  BlockId(blkId),
  PayloadSize(payloadSize)
{
    // If packet is a command and the ID is automatic, we increment the last ID, for the next packet.
    if (!Modifiers.IsResponse && PacketId == s_lastPktId) { ++s_lastPktId; }
}

constexpr PacketHeader::PacketHeader(PacketHeader::RawData data)
: PacketId(AsciiToT<decltype(PacketId)>(&data[s_packetIdOffset])),
  CommandId(AsciiToT<decltype(CommandId)>(&data[s_commandIdOffset])),
  Modifiers(AsciiToT<uint8_t>(&data[s_modifiersOffset])),
  BlockId(AsciiToT<decltype(BlockId)>(&data[s_blockIdOffset])),
  PayloadSize(AsciiToT<decltype(PayloadSize)>(&data[s_payloadSizeOffset]))
{
    if (data.size() != s_headerSize) { throw MissingDataException(data.data(), data.length(), "header"); }
    else if (data[s_sohOffset] != s_sohFlag)
    {
        throw BadDelimiterException(data.data(), data.length(), "header", s_sohFlag);
    }
}

constexpr PacketHeader::operator std::vector<uint8_t>() const noexcept
{
    std::vector<uint8_t> out;
    out.reserve(s_headerSize);
    out.push_back(s_sohFlag);

    auto pktId = TToAscii(PacketId);
    out.insert(out.end(), pktId.begin(), pktId.end());

    auto cmdId = TToAscii(CommandId);
    out.insert(out.end(), cmdId.begin(), cmdId.end());

    auto modifiers = TToAscii(static_cast<uint8_t>(Modifiers));
    out.insert(out.end(), modifiers.begin(), modifiers.end());

    auto blockId = TToAscii(BlockId);
    out.insert(out.end(), blockId.begin(), blockId.end());

    auto payloadSize = TToAscii(PayloadSize);
    out.insert(out.end(), payloadSize.begin(), payloadSize.end());

    return out;
}


Packet::Packet(
  cmd_id_t cmdId, blk_id_t blkId, const std::vector<uint8_t>& data, bool isResp, bool isLast, pkt_id_t pktId)
: Header(pktId, cmdId, PacketModifiers {isResp, isLast}, blkId, static_cast<uint8_t>(data.size())), Payload(data), Crc()
{
}

Packet::Packet(const std::vector<uint8_t>& raw)
{
    if (raw.size() <= s_minimumPacketSize) { throw MissingDataException(raw.data(), raw.size(), "packet"); }
    else if (raw[0] != s_packetStartFlag)
    {
        throw BadDelimiterException(raw.data(), raw.size(), "packet", s_packetStartFlag);
    }
    else
    {
        Header = PacketHeader({&raw[1], PacketHeader::s_headerSize});
        // Make sure payload start is there.
        if (raw[s_payloadStartOffset] != s_payloadStartFlag)
        {
            throw BadDelimiterException(raw.data(), raw.size(), "payload", s_payloadStartFlag);
        }

        // Find the end of the payload.
        size_t realPayloadSize       = (Header.PayloadSize * s_charsPerBytes);
        size_t expectedPayloadEndIdx = s_payloadStartOffset + realPayloadSize + 1;
        if (raw[expectedPayloadEndIdx] != s_payloadEndFlag)
        {
            throw BadPayloadException(raw.data(), raw.size(), expectedPayloadEndIdx);
        }

        Payload.reserve(Header.PayloadSize);
        for (size_t i = s_payloadStartOffset + 1; i < expectedPayloadEndIdx; i += 2)
        {
            Payload.push_back(AsciiToT<uint8_t>(&raw[i]));
        }

        // TODO: Verify CRC.
        Crc = AsciiToT<decltype(Crc)>((&raw[expectedPayloadEndIdx]) + 1);
    }
}

Packet::operator std::vector<uint8_t>() const noexcept
{
    std::vector<uint8_t> out;
    out.reserve(PacketHeader::s_headerSize + sizeof(s_payloadStartFlag) + (Payload.size() * 2) +
                sizeof(s_payloadEndFlag) + SizeInChars<decltype(Crc)>() + sizeof(s_packetEndFlag));

    out.push_back(s_packetStartFlag);

    auto header = static_cast<std::vector<uint8_t>>(Header);
    out.insert(out.end(), header.begin(), header.end());

    out.push_back(s_payloadStartFlag);
    for (auto&& byte : Payload)
    {
        auto serialized = TToAscii(byte);
        out.insert(out.end(), serialized.begin(), serialized.end());
    }
    out.push_back(s_payloadEndFlag);

    auto crc = TToAscii(Crc);
    out.insert(out.end(), crc.begin(), crc.end());

    out.push_back(s_packetEndFlag);
    BR_APP_DEBUG("Serialized Packet: {}", std::span {out.begin(), out.end()});

    return out;
}

}    // namespace Frasy::Communication
