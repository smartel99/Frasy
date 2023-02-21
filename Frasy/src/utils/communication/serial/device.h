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

#include "../../commands/dispatcher.h"
#include "../../commands/event.h"
#include "enumerator.h"
#include "exceptions.h"
#include "packet.h"
#include "response.h"
#include "types.h"
#include "utils/commands/description/command.h"

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
        m_label    = std::move(o.m_label);
        m_rxBuff   = std::move(o.m_rxBuff);
        m_pending  = std::move(o.m_pending);
        m_info     = std::move(o.m_info);
        m_commands = std::move(o.m_commands);
        m_structs  = std::move(o.m_structs);
        m_enums    = std::move(o.m_enums);
        m_ready    = o.m_ready;

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

    void Open();
    void Close();
    void Reset();
    bool Log(bool enable);

    [[nodiscard]] const Actions::Identify::Info&       GetInfo() const noexcept { return m_info; }
    [[nodiscard]] const std::vector<Actions::Command>& GetCommands() const noexcept { return m_commands; }

    [[nodiscard]] bool        IsOpen() const noexcept { return m_device.isOpen(); }
    [[nodiscard]] std::string GetPort() const noexcept { return m_device.getPort(); }

    ResponsePromise& Transmit(const Packet& pkt)
    {
        std::vector<uint8_t> data = static_cast<std::vector<uint8_t>>(pkt);
        m_device.write(data);
        std::lock_guard lock {m_lock};
        auto&& [it, success] = m_pending.insert_or_assign(pkt.Header.TransactionId, std::move(ResponsePromise {}));
        return it->second;
    }

private:
    void CheckForPackets();
    void GetCapabilities();

private:
    std::string    m_label;
    serial::Serial m_device = {};    //!< The physical communication interface.

    bool m_ready = false;

    std::mutex  m_lock;
    bool        m_shouldRun = true;
    std::thread m_rxThread;
    std::string m_rxBuff = {};    //!< Buffer where the received data go.

    Actions::Identify::Info       m_info     = {};
    std::vector<Actions::Command> m_commands = {};
    std::vector<Type::Struct>     m_structs  = {};
    std::vector<Type::Enum>       m_enums    = {};

    std::unordered_map<trs_id_t, ResponsePromise> m_pending;
};
}    // namespace Frasy::Communication

#endif    // FRASY_UTILS_COMM_SERIAL_DEVICE_H
