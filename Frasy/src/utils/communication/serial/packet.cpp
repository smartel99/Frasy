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

#include "exceptions.h"
#include "utils/misc/char_conv.h"
#include "utils/misc/serializer.h"

#include <Brigerad/Core/Log.h>

namespace Frasy::Communication
{
PacketHeader::PacketHeader(trs_id_t trsId, cmd_id_t cmdId, PacketModifiers mods, payload_size_t payloadSize)
: TransactionId(MakeTransactionId(trsId)), CommandId(cmdId), Modifiers(mods), PayloadSize(payloadSize)
{
    // If packet is a command and the ID is automatic, we increment the last ID, for the next
    // packet.
    if (!Modifiers.IsResponse && TransactionId == s_lastTrsId) { ++s_lastTrsId; }
}

constexpr PacketHeader::PacketHeader(PacketHeader::RawData data)
: TransactionId(AsciiToT<decltype(TransactionId)>(&data[s_transactionIdOffset])),
  CommandId(AsciiToT<decltype(CommandId)>(&data[s_commandIdOffset])),
  Modifiers(AsciiToT<uint8_t>(&data[s_modifiersOffset])),
  PayloadSize(AsciiToT<decltype(PayloadSize)>(&data[s_payloadSizeOffset]))
{
    if (data.size() != s_headerSize) { throw MissingDataException(data.data(), data.length(), "header"); }
}

[[nodiscard]] std::vector<uint8_t> PacketHeader::ToAscii() const noexcept
{
    std::vector<uint8_t> out;
    out.reserve(s_headerSize);

    auto pktId = TToAscii(TransactionId);
    out.insert(out.end(), pktId.begin(), pktId.end());

    auto cmdId = TToAscii(CommandId);
    out.insert(out.end(), cmdId.begin(), cmdId.end());

    auto modifiers = TToAscii(static_cast<uint8_t>(Modifiers));
    out.insert(out.end(), modifiers.begin(), modifiers.end());

    auto payloadSize = TToAscii(PayloadSize);
    out.insert(out.end(), payloadSize.begin(), payloadSize.end());

    return out;
}
PacketHeader::operator std::vector<uint8_t>() const noexcept
{
    std::vector<uint8_t> out;
    out.reserve(s_headerSize);

    auto pktId = Serialize(TransactionId == AUTOMATIC_TRANSACTION_ID ? MakeTransactionId(AUTOMATIC_TRANSACTION_ID)
                                                                     : TransactionId);
    out.insert(out.end(), pktId.begin(), pktId.end());

    auto cmdId = Serialize(CommandId);
    out.insert(out.end(), cmdId.begin(), cmdId.end());

    auto modifiers = Serialize(static_cast<uint8_t>(Modifiers));
    out.insert(out.end(), modifiers.begin(), modifiers.end());

    auto payloadSize = Serialize(PayloadSize);
    out.insert(out.end(), payloadSize.begin(), payloadSize.end());

    return out;
}

bool PacketHeader::operator==(const PacketHeader& other) const
{
    return TransactionId == other.TransactionId &&    //
           CommandId == other.CommandId &&            //
           Modifiers == other.Modifiers &&            //
           PayloadSize == other.PayloadSize;
}

[[nodiscard]] trs_id_t PacketHeader::MakeTransactionId(trs_id_t id) const
{
    static uint32_t s_lastId = 0;
    if (id == AUTOMATIC_TRANSACTION_ID) { id = 0xF000 | s_lastId++; }
    return id;
}


Packet::Packet(cmd_id_t cmdId, const std::vector<uint8_t>& data, bool isResp, trs_id_t trsId, uint32_t crc)
: Header(trsId, cmdId, PacketModifiers(isResp), static_cast<uint8_t>(data.size())), Payload(data), m_crc(crc)
{
}

Packet::Packet(const std::vector<uint8_t>& raw)
{
    if (raw.size() <= s_minimumPacketSize) { throw MissingDataException(raw.data(), raw.size(), "packet"); }
    if (raw[0] != s_packetStartFlag)
    {
        throw BadDelimiterException(raw.data(), raw.size(), "packet", s_packetStartFlag);
    }
    if (raw[s_sohOffset] != s_sohFlag) { throw BadDelimiterException(raw.data(), raw.size(), "header", s_sohFlag); }

    Header = PacketHeader({&raw[s_headerOffset], PacketHeader::s_headerSize});

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

    m_crc    = AsciiToT<decltype(m_crc)>((&raw[expectedPayloadEndIdx]) + 1);
    auto crc = crc32_calculate({std::vector<uint8_t>(Header), Payload});
    if (m_crc != crc) { throw BadCrcException(raw.data(), raw.size(), m_crc, crc); }
}

Packet::operator std::vector<uint8_t>() const noexcept
{
    std::vector<uint8_t> out;
    out.reserve(sizeof(s_packetStartFlag) +             // START PACKET     (1)
                sizeof(s_sohFlag) +                     // START HEADER     (1)
                PacketHeader::s_headerSize +            // HEADER           (18 * 2)
                sizeof(s_payloadStartFlag) +            // START PAYLOAD    (1)
                (Payload.size() * s_charsPerBytes) +    // PAYLOAD          (X * 2)
                sizeof(s_payloadEndFlag) +              // END PAYLOAD      (1)
                SizeInChars<decltype(m_crc)>() +        // CRC              (4 * 2)
                sizeof(s_packetEndFlag)                 // END TRANSMISSION (1)
    );                                                  // TOTAL            (49 + X * 2)

    out.push_back(s_packetStartFlag);
    out.push_back(s_sohFlag);

    auto header = Header.ToAscii();
    out.insert(out.end(), header.begin(), header.end());

    out.push_back(s_payloadStartFlag);
    for (auto&& byte : Payload)
    {
        auto serialized = TToAscii(byte);
        out.insert(out.end(), serialized.begin(), serialized.end());
    }
    out.push_back(s_payloadEndFlag);

    auto crc       = crc32_calculate({std::vector<uint8_t>(Header), Payload});
    auto ascii_crc = TToAscii(crc);
    out.insert(out.end(), ascii_crc.begin(), ascii_crc.end());

    out.push_back(s_packetEndFlag);

    return out;
}

[[nodiscard]] bool Packet::operator==(const Packet& other) const
{
    return Header == other.Header && Payload == other.Payload;
}

}    // namespace Frasy::Communication
