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
#include "utils/commands/built_in/command_info/reply.h"
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
        bool reopen = false;
        if (o.m_device.isOpen())
        {
            reopen = true;
            o.Close();
        }

        m_label       = std::move(o.m_label);
        m_rxBuff      = std::move(o.m_rxBuff);
        m_pending     = std::move(o.m_pending);
        m_info        = std::move(o.m_info);
        m_commands    = std::move(o.m_commands);
        m_typeManager = std::move(o.m_typeManager);
        m_log         = o.m_log;
        m_ready       = o.m_ready;

        serial::Timeout timeout = serial::Timeout::simpleTimeout(10);
        m_device.setTimeout(timeout);
        m_device.setBaudrate(460800);
        m_device.setPort(o.m_device.getPort());
        if (reopen) { Open(); }
        return *this;
    }

    void               Open();
    void               Close();
    void               Reset();
    [[nodiscard]] bool GetLog() const;
    void               SetLog(bool enable);

    [[nodiscard]] const Actions::Identify::Info& GetInfo() const noexcept { return m_info; }
    [[nodiscard]] const std::unordered_map<cmd_id_t, Actions::CommandInfo::Reply>& GetCommands() const noexcept
    {
        return m_commands;
    }
    [[nodiscard]] const std::unordered_map<type_id_t, Type::Struct>& GetStructs() const noexcept
    {
        return m_typeManager.GetStructs();
    }
    [[nodiscard]] const std::unordered_map<type_id_t, Type::Enum>& GetEnums() const noexcept
    {
        return m_typeManager.GetEnums();
    }
    [[nodiscard]] const Type::Manager & GetTypeManager() const noexcept { return m_typeManager; }


    [[nodiscard]] bool        IsOpen() const noexcept { return m_device.isOpen(); }
    [[nodiscard]] std::string GetPort() const noexcept { return m_device.getPort(); }
    [[nodiscard]] bool        Ready() const noexcept { return m_ready; }
    [[nodiscard]] bool        Enabled() const noexcept { return m_enabled; }

    ResponsePromise& Transmit(const Packet& pkt)
    {
        std::vector<uint8_t> data = static_cast<std::vector<uint8_t>>(pkt);
        std::lock_guard      txLock {m_txLock};
        m_device.write(data);
        std::lock_guard promiseLock {m_promiseLock};
        auto&& [it, success] = m_pending.insert_or_assign(pkt.Header.TransactionId, std::move(ResponsePromise {}));
        return it->second;
    }

private:
    void CheckForPackets();
    void GetCapabilities();

private:
    std::string    m_label;
    serial::Serial m_device = {};    //!< The physical communication interface.

    bool m_ready   = false;
    bool m_enabled = true;

    std::mutex  m_txLock;
    std::mutex  m_promiseLock;
    bool        m_shouldRun = true;
    std::thread m_rxThread;
    std::string m_rxBuff = {};    //!< Buffer where the received data go.

    Actions::Identify::Info                                   m_info     = {};
    std::unordered_map<cmd_id_t, Actions::CommandInfo::Reply> m_commands = {};
    bool                                                      m_log      = false;
    Frasy::Type::Manager                                      m_typeManager;

    std::unordered_map<trs_id_t, ResponsePromise> m_pending;
};
}    // namespace Frasy::Communication

#endif    // FRASY_UTILS_COMM_SERIAL_DEVICE_H
