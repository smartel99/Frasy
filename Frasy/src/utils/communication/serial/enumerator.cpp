/**
 * @file    enumerator.cpp
 * @author  Samuel Martel
 * @date    2022-12-14
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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "enumerator.h"

#include "packet.h"

#include <Brigerad/Core/Log.h>
#include <optional>
#include <serial/serial.h>

namespace Frasy::Serial {

static constexpr const char* s_tag = "Enumerator";

static std::optional<Actions::Identify::Info> IdentifyPort(const serial::PortInfo& info);

std::vector<DeviceInfo> EnumerateInstrumentationCards()
{
    std::vector<DeviceInfo>       devices;
    std::vector<serial::PortInfo> ports = serial::list_ports();

    BR_LOG_DEBUG(s_tag, "Found {} serial devices", ports.size());

    for (auto&& port : ports)
    {
        auto info = IdentifyPort(port);
        if (info) { devices.emplace_back(DeviceInfo {port.port, *info}); }
    }

    return devices;
}

std::optional<Actions::Identify::Info> IdentifyPort(const serial::PortInfo& info)
{
    BR_LOG_DEBUG(s_tag,
                 "Attempting to identify device on port '{}' (des: {}, id: {})",
                 info.port,
                 info.description,
                 info.hardware_id);

    Packet pkt;
    pkt.Header.CommandId = static_cast<cmd_id_t>(Actions::CommandId::Identify);

    std::string packetEnd = std::string(1, Packet::s_packetEndFlag);
    try
    {
        serial::Serial port = serial::Serial(info.port, 460800, serial::Timeout::simpleTimeout(500));
        if (!port.isOpen()) { port.open(); }

        port.write(static_cast<std::vector<uint8_t>>(pkt));
        std::string resp = port.readline(Packet::s_maximumPacketSize, packetEnd);
        port.close();    // We must close serial because it will be reopened later by SerialDevice
        return Actions::Identify::Info(Packet({resp.begin(), resp.end()}).FromPayload<Actions::Identify::Reply>());
    }
    catch (std::exception& e)
    {
        BR_LOG_ERROR(s_tag, "While identifying {}: {}", info.port, e.what());
    }

    return {};
}

}    // namespace Frasy::Communication
