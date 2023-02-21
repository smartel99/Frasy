/**
 * @file    packet.h
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

#ifndef FRASY_UTILS_COMM_SERIAL_PACKET_H
#define FRASY_UTILS_COMM_SERIAL_PACKET_H

#include "types.h"
#include "utils/commands/built_in/id.h"
#include "utils/misc/char_conv.h"
#include "utils/misc/crc32.h"
#include "utils/misc/deserializer.h"
#include "utils/misc/serializer.h"
#include "utils/misc/type_size.h"

#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Frasy::Communication
{

struct PacketHeader
{
    using RawData = std::basic_string_view<uint8_t>;

    trs_id_t        TransactionId = {};
    cmd_id_t        CommandId     = {};
    PacketModifiers Modifiers;
    payload_size_t  PayloadSize = {};

    constexpr PacketHeader() noexcept = default;
    constexpr PacketHeader(trs_id_t pktId, cmd_id_t cmdId, PacketModifiers mods, payload_size_t payloadSize);
    explicit constexpr PacketHeader(RawData data);

    [[nodiscard]] std::vector<uint8_t> ToAscii() const noexcept;
    [[nodiscard]] explicit             operator std::vector<uint8_t>() const noexcept;
    [[nodiscard]] bool                 operator==(const PacketHeader& other) const;

private:
    inline static trs_id_t s_lastTrsId = 0;

    static constexpr size_t s_transactionIdOffset = std::distance(RawData {}.begin(), RawData {}.begin());
    static constexpr size_t s_commandIdOffset     = s_transactionIdOffset + SizeInChars<decltype(TransactionId)>();
    static constexpr size_t s_modifiersOffset     = s_commandIdOffset + SizeInChars<decltype(CommandId)>();
    static constexpr size_t s_payloadSizeOffset   = s_modifiersOffset + SizeInChars<uint8_t>();

public:
    static constexpr size_t s_headerSize = s_payloadSizeOffset + SizeInChars<decltype(PayloadSize)>();

    static_assert(s_headerSize == SizeInChars<decltype(TransactionId)>() +    //
                                    SizeInChars<decltype(CommandId)>() +      //
                                    SizeInChars<uint8_t>() +                  //
                                    SizeInChars<decltype(PayloadSize)>());
};

/**
 * @struct Packet
 *
 * <a href="../../../../../doc/architecture/communication/serial/packets.md">Documentation</a>
 */
struct Packet
{

public:
    Packet() noexcept = default;
    explicit Packet(const std::vector<uint8_t>& raw);
    Packet(cmd_id_t                    cmdId,
           const std::vector<uint8_t>& data,
           bool                        isResp = false,
           trs_id_t                    trsId  = AUTOMATIC_TRANSACTION_ID,
           uint32_t                    crc    = 0);

    [[nodiscard]] explicit operator std::vector<uint8_t>() const noexcept;
    [[nodiscard]] bool     operator==(const Packet& other) const;

    PacketHeader         Header  = {};
    std::vector<uint8_t> Payload = {};

private:
    uint32_t m_crc = 0;

public:
    [[nodiscard]] uint32_t Crc() const { return m_crc; }
    [[nodiscard]] uint32_t ComputeCrc() const { return crc32_calculate({std::vector<uint8_t>(Header), Payload}); }
    [[nodiscard]] bool IsCrcValid() const { return m_crc == crc32_calculate({std::vector<uint8_t>(Header), Payload}); }

    static Packet Request(cmd_id_t cmdId)
    {
        Packet pkt;
        pkt.Header.CommandId = cmdId;
        pkt.m_crc            = pkt.ComputeCrc();
        return pkt;
    }
    static Packet Request(Actions::CommandId cmdId) { return Request(static_cast<cmd_id_t>(cmdId)); }


    static Packet Reply(const Packet& og, cmd_id_t cmdId = INVALID_COMMAND_ID)
    {
        Packet pkt;
        pkt.Header                      = og.Header;
        pkt.Header.Modifiers.IsResponse = true;
        if (cmdId != INVALID_COMMAND_ID) { pkt.Header.CommandId = cmdId; }
        return pkt;
    }

    template<typename T>
    Packet& MakePayload(const T& t)
        requires requires { Serialize(t); }
    {
        Payload            = Serialize(t);
        Header.PayloadSize = Payload.size();
        m_crc              = ComputeCrc();
        return *this;
    }

    template<typename T>
    T FromPayload() const
    {
        return Deserialize<T>(Payload.begin(), Payload.end());
    }

    static constexpr size_t  s_charsPerBytes   = 2;         //!< Each byte of data is 2 characters.
    static constexpr uint8_t s_packetStartFlag = '\x16';    //!< Value for SYN (Synchronization).

    static constexpr uint8_t s_sohFlag      = '\x01';    //!< ASCII for Start of Heading.
    static constexpr size_t  s_sohOffset    = sizeof(s_packetStartFlag);
    static constexpr size_t  s_headerOffset = s_sohOffset + sizeof(s_sohFlag);

    static constexpr size_t  s_payloadStartOffset = s_headerOffset + PacketHeader::s_headerSize;
    static constexpr uint8_t s_payloadStartFlag   = 0x02;    //!< Value for STX (Start of Text).
    static constexpr uint8_t s_payloadEndFlag     = 0x03;
    static constexpr uint8_t s_packetEndFlag      = 0x04;

    static constexpr size_t s_maximumPayloadSize = std::numeric_limits<payload_size_t>::max();
    static constexpr size_t s_minimumPacketSize  = sizeof(s_packetStartFlag) +        //
                                                  sizeof(s_sohFlag) +                 //
                                                  PacketHeader::s_headerSize +        //
                                                  sizeof(s_payloadStartFlag) +        //
                                                  sizeof(s_payloadEndFlag) +          //
                                                  SizeInChars<decltype(m_crc)>() +    //
                                                  sizeof(s_packetEndFlag);
    static constexpr size_t s_maximumPacketSize = s_minimumPacketSize + (s_maximumPayloadSize * s_charsPerBytes);
};


}    // namespace Frasy::Communication


#endif    // FRASY_UTILS_COMM_SERIAL_PACKET_H
