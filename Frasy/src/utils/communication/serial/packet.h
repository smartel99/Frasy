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

#include "../../misc/char_conv.h"
#include "../../misc/deserializer.h"
#include "../../misc/serializer.h"
#include "../../misc/type_size.h"
#include "types.h"

#include <optional>
#include <spdlog/fmt/fmt.h>
#include <string>
#include <string_view>
#include <vector>

namespace Frasy::Communication
{

struct PacketHeader
{
    using RawData = std::basic_string_view<uint8_t>;

    pkt_id_t        PacketId    = {};
    cmd_id_t        CommandId   = {};
    PacketModifiers Modifiers   = PacketModifiers {0};
    blk_id_t        BlockId     = {};
    payload_size_t  PayloadSize = {};

    constexpr PacketHeader() noexcept = default;
    constexpr explicit PacketHeader(RawData data);
    constexpr explicit PacketHeader(
      pkt_id_t pktId, cmd_id_t cmdId, PacketModifiers mods, blk_id_t blkId, payload_size_t payloadSize);

    [[nodiscard]] constexpr explicit operator std::vector<uint8_t>() const noexcept;

private:
    inline static pkt_id_t s_lastPktId = 0;

    static constexpr uint8_t s_sohFlag           = '\x01';    //!< ASCII for Start of Heading.
    static constexpr size_t  s_sohOffset         = std::distance(RawData {}.begin(), RawData {}.begin());
    static constexpr size_t  s_packetIdOffset    = s_sohOffset + sizeof(s_sohFlag);
    static constexpr size_t  s_commandIdOffset   = s_packetIdOffset + SizeInChars<decltype(PacketId)>();
    static constexpr size_t  s_modifiersOffset   = s_commandIdOffset + SizeInChars<decltype(CommandId)>();
    static constexpr size_t  s_blockIdOffset     = s_modifiersOffset + SizeInChars<uint8_t>();
    static constexpr size_t  s_payloadSizeOffset = s_blockIdOffset + SizeInChars<decltype(BlockId)>();

public:
    static constexpr size_t s_headerSize = s_payloadSizeOffset + SizeInChars<decltype(PayloadSize)>();

    static_assert(s_headerSize == sizeof(s_sohFlag) + SizeInChars<decltype(PacketId)>() +
                                    SizeInChars<decltype(CommandId)>() + SizeInChars<uint8_t>() +
                                    SizeInChars<decltype(BlockId)>() + SizeInChars<decltype(PayloadSize)>());
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
           blk_id_t                    blkId,
           const std::vector<uint8_t>& data,
           bool                        isResp = false,
           bool                        isLast = false,
           pkt_id_t                    pktId  = AUTOMATIC_PACKET_ID);
    [[nodiscard]] explicit operator std::vector<uint8_t>() const noexcept;

    PacketHeader         Header  = {};
    std::vector<uint8_t> Payload = {};

    uint32_t Crc = 0;

    template<Serializable T>
    void MakePayload(const T& t)
    {
        Payload = Serialize(t);
    }

    template<typename T>
    T FromPayload()
    {
        return Deserialize<T>(Payload.begin(), Payload.end());
    }

    static constexpr size_t  s_charsPerBytes   = 2;         //!< Each byte of data is 2 characters.
    static constexpr uint8_t s_packetStartFlag = '\x16';    //!< Value for SYN (Synchronization).

    static constexpr size_t  s_payloadStartOffset = PacketHeader::s_headerSize + sizeof(s_packetStartFlag);
    static constexpr uint8_t s_payloadStartFlag   = 0x02;    //!< Value for STX (Start of Text).
    static constexpr uint8_t s_payloadEndFlag     = 0x03;
    static constexpr uint8_t s_packetEndFlag      = 0x04;

    static constexpr size_t s_maximumPayloadSize = std::numeric_limits<payload_size_t>::max();
    static constexpr size_t s_minimumPacketSize  = sizeof(s_packetStartFlag) + PacketHeader::s_headerSize +
                                                  sizeof(s_payloadStartFlag) + sizeof(s_payloadEndFlag) +
                                                  SizeInChars<decltype(Crc)>() + sizeof(s_packetEndFlag);
    static constexpr size_t s_maximumPacketSize = s_minimumPacketSize + (s_maximumPayloadSize * s_charsPerBytes);
};


}    // namespace Frasy::Communication

template<>
struct fmt::formatter<Frasy::Communication::PacketHeader>
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin())
    {
        auto it  = ctx.begin();
        auto end = ctx.end();
        if (it != end && *it != '}') { throw fmt::format_error("No format supported"); }
        return it;
    }

    template<typename FormatContext>
    auto format(const Frasy::Communication::PacketHeader& header, FormatContext& ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(),
                              "PacketId: {}, CmdId: {}\n\r\tIsResponse: {}, IsLastPacket: {}"
                              "\n\r\tBlockId: {}, PayloadSize: {}",
                              header.PacketId,
                              header.CommandId,
                              header.Modifiers.IsResponse,
                              header.Modifiers.IsLastPacket,
                              header.BlockId,
                              header.PayloadSize);
    }
};

template<>
struct fmt::formatter<Frasy::Communication::Packet>
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin())
    {
        auto it  = ctx.begin();
        auto end = ctx.end();
        if (it != end || *it != '}') { throw fmt::format_error("No format supported"); }
        return it;
    }

    template<typename FormatContext>
    auto format(const Frasy::Communication::Packet& pkt, FormatContext& ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(),
                              "Header: {}\n\r\tData: {:02X}\n\r\tCRC: {:#08x}",
                              pkt.Header,
                              fmt::join(pkt.Payload, ", "),
                              pkt.Crc);
    }
};

#endif    // FRASY_UTILS_COMM_SERIAL_PACKET_H
