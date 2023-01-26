/**
 * @file    device.h
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

#ifndef FRASY_UTILS_COMM_SERIAL_DEVICE_H
#define FRASY_UTILS_COMM_SERIAL_DEVICE_H
#include "../../../instrumentation_card/command_description.h"
#include "../../commands/dispatcher.h"
#include "../../commands/event.h"
#include "enumerator.h"
#include "exceptions.h"
#include "packet.h"
#include "response.h"
#include "types.h"

#include <Brigerad/Core/Log.h>
#include <Brigerad/Core/Time.h>
#include <functional>
#include <mutex>
#include <serial/serial.h>
#include <thread>
#include <vector>

namespace Frasy::Communication
{
/**
 * Handles background reception of data from the serial device, as well as the transmission.
 *
 * Received packets are forwarded to the
 */
class SerialDevice
{
public:
    SerialDevice() noexcept = default;
    SerialDevice(SerialDevice&& o) noexcept { *this = std::move(o); }
    explicit SerialDevice(DeviceInfo info, bool open = true)
    : m_label(fmt::format("UART {}", info.ComPort)),
      m_device(info.ComPort, 460800, serial::Timeout::simpleTimeout(10)),
      m_info(info.Info)
    {
        if (open) { Open(); }
    }
    ~SerialDevice() { Close(); }

    SerialDevice& operator=(SerialDevice&& o) noexcept
    {
        m_label             = std::move(o.m_label);
        m_rxBuff            = std::move(o.m_rxBuff);
        m_pending           = std::move(o.m_pending);
        m_info              = std::move(o.m_info);
        m_supportedCommands = std::move(o.m_supportedCommands);

        bool reopen = false;
        if (o.m_device.isOpen())
        {
            reopen = true;
            o.Close();
        }
        m_device.setPort(o.m_device.getPort());
        if (reopen) { Open(); }
        return *this;
    }

    void Open()
    {
        // If already open, re-open.
        if (m_device.isOpen()) { Close(); }

        try
        {
            m_device.open();
        }
        catch (std::exception& e)
        {
            BR_LOG_ERROR(m_label, "While opening '{}': {}", m_device.getPort(), e.what());
            return;
        }

        m_shouldRun = true;
        m_rxThread  = std::thread(
          [this]()
          {
              BR_LOG_INFO(m_label, "Started RX listener on '{}'", m_device.getPort());
              std::string endOfPacket = std::string(1, Packet::s_packetEndFlag);
              while (m_shouldRun)
              {
                  try
                  {
                      std::string read = m_device.readline(Packet::s_maximumPacketSize, endOfPacket);
                      m_rxBuff += read;
                  }
                  catch (std::exception& e)
                  {
                      BR_LOG_ERROR(m_label, "An error occurred in the listener thread: {}", e.what());
                      break;
                  }
                  CheckForPackets();
              }
              BR_LOG_INFO(m_label, "RX listener terminated on '{}'", m_device.getPort());
          });

        EnumerateDeviceCapabilities();
    }

    void Close();

    [[nodiscard]] const PrettyInstrumentationCardInfo&                   GetInfo() const noexcept { return m_info; }
    [[nodiscard]] const std::vector<Instrumentation::CommandDescription> GetSupportedCommands() const noexcept
    {
        return m_supportedCommands;
    }

    [[nodiscard]] bool        IsOpen() const noexcept { return m_device.isOpen(); }
    [[nodiscard]] std::string GetPort() const noexcept { return m_device.getPort(); }

    TransmissionCallbacks& Transmit(const Packet& pkt)
    {
        std::vector<uint8_t> data = static_cast<std::vector<uint8_t>>(pkt);
        m_device.write(data);
        std::lock_guard lock(m_lock);
        auto&& [it, success] = m_pending.insert_or_assign(pkt.Header.PacketId, std::move(ResponsePromise {}));
        return it->second.Callbacks;
    }

private:
    void CheckForPackets();

    void EnumerateDeviceCapabilities();

private:
    std::string    m_label;
    serial::Serial m_device = {};    //!< The physical communication interface.

    bool m_ready = false;

    std::mutex  m_lock;
    bool        m_shouldRun = true;
    std::thread m_rxThread;
    std::string m_rxBuff = {};    //!< Buffer where the received data go.

    PrettyInstrumentationCardInfo m_info = {};

    std::unordered_map<pkt_id_t, ResponsePromise> m_pending;

    std::vector<Instrumentation::CommandDescription> m_supportedCommands;
};
}    // namespace Frasy::Communication

#endif    // FRASY_UTILS_COMM_SERIAL_DEVICE_H
