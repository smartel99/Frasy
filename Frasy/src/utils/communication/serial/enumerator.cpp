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
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */
#include "enumerator.h"

#include "exceptions.h"
#include "packet.h"

#include <Brigerad/Core/Log.h>
#include <future>
#include <optional>
#include <serial/serial.h>

namespace Frasy::Communication
{

static constexpr const char* s_tag = "Enumerator";

static std::optional<Actions::Identify::Info> IdentifyPort(const serial::PortInfo& info);

std::vector<DeviceInfo> EnumerateInstrumentationCards()
{
    std::vector<DeviceInfo>       devices;
    std::vector<serial::PortInfo> ports = serial::list_ports();

    BR_LOG_DEBUG(s_tag, "Found {} serial devices", ports.size());

    std::vector<std::future<std::pair<serial::PortInfo, std::optional<Actions::Identify::Info>>>> workers;

    for (auto&& port : ports)
    {
        workers.push_back(std::async(
          std::launch::async,
          [](const serial::PortInfo& portInfo) -> std::pair<serial::PortInfo, std::optional<Actions::Identify::Info>>
          {
              static constexpr size_t s_maxAttempts = 5;
              size_t                  attempts      = s_maxAttempts;
              do {
                  auto info = IdentifyPort(portInfo);
                  if (info) { return std::make_pair(portInfo, info); }
              } while (attempts-- != 0);
              return std::make_pair(portInfo, std::nullopt);
          },
          port));
    }

    for (auto&& worker : workers)
    {
        auto [portInfo, boardInfo] = worker.get();
        if (boardInfo) { devices.emplace_back(DeviceInfo {portInfo.port, *boardInfo}); }
    }

    //    for (auto&& port : ports)
    //    {
    //        auto info = IdentifyPort(port);
    //        if (info) { devices.emplace_back(DeviceInfo {port.port, *info}); }
    //    }

    return devices;
}

std::optional<Actions::Identify::Info> IdentifyPort(const serial::PortInfo& info)
{
    BR_LOG_DEBUG(
      s_tag,
      "Attempting to identify device on port '{}' (des: {}, id: {})",
      info.port,
      info.description,
      info.hardware_id);

    Packet pkt;
    pkt.Header.CommandId = static_cast<cmd_id_t>(Actions::CommandId::Identify);

    static std::string packetEnd = std::string(1, Packet::s_packetEndFlag);
    try
    {
        serial::Serial port = serial::Serial(info.port, 460800, serial::Timeout::simpleTimeout(500));
        if (!port.isOpen()) { port.open(); }

        port.write(static_cast<std::vector<uint8_t>>(pkt));
        std::string resp = port.readline(Packet::s_maximumPacketSize, packetEnd);
        port.close();    // We must close serial because it will be reopened later by SerialDevice
        return Actions::Identify::Info(Packet({resp.begin(), resp.end()}).FromPayload<Actions::Identify::Reply>());
    }
    catch (Frasy::Communication::BasePacketException& e)
    {
        // Device on the com port is (probably) supported, but there was an error with the packet.
        BR_LOG_ERROR(s_tag, "While identifying {}: {}", info.port, e.what());
    }
    catch (std::exception& e)
    {
        // Error with the com port itself.
        BR_LOG_WARN(s_tag, "While identifying {}: {}", info.port, e.what());
    }

    return {};
}

}    // namespace Frasy::Communication
