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

namespace Frasy::Communication {
/**
 * Handles background reception of data from the serial device, as well as the transmission.
 *
 * Received packets are forwarded to the
 */
class SerialDevice {
public:
    SerialDevice() noexcept = default;
    SerialDevice(SerialDevice&& o) noexcept { *this = std::move(o); }
    explicit SerialDevice(DeviceInfo info, bool open = true);
    ~SerialDevice() { close(); }

    SerialDevice& operator=(SerialDevice&& o) noexcept;

    void               open();
    void               close();
    void               reset();
    [[nodiscard]] bool getLog() const;
    void               setLog(bool enable);

    [[nodiscard]] const Actions::Identify::Info& getInfo() const noexcept { return m_info; }
    [[nodiscard]] const std::unordered_map<cmd_id_t, Actions::CommandInfo::Reply>& getCommands() const noexcept
    {
        return m_commands;
    }
    [[nodiscard]] const std::unordered_map<type_id_t, Type::Struct>& getStructs() const noexcept
    {
        return m_typeManager.GetStructs();
    }
    [[nodiscard]] const std::unordered_map<type_id_t, Type::Enum>& getEnums() const noexcept
    {
        return m_typeManager.GetEnums();
    }
    [[nodiscard]] const Type::Manager& GetTypeManager() const noexcept { return m_typeManager; }


    [[nodiscard]] bool        isOpen() const noexcept { return m_device.isOpen(); }
    [[nodiscard]] std::string getPort() const noexcept { return m_device.getPort(); }
    [[nodiscard]] bool        ready() const noexcept { return m_ready; }
    [[nodiscard]] bool        enabled() const noexcept { return m_enabled; }

    ResponsePromise& transmit(Packet pkt);

    [[nodiscard]] std::vector<trs_id_t> getPendingTransactions();

private:
    void checkForPackets();
    void getCapabilities();
    void cleanerTask();

private:
    std::string    m_label;
    serial::Serial m_device;    //!< The physical communication interface.

    std::thread   m_cleanerThread;
    volatile bool m_cleanerRun = false;

    bool m_ready   = false;
    bool m_enabled = true;

    std::mutex  m_promiseLock;
    bool        m_shouldRun = true;
    std::thread m_rxThread;
    std::string m_rxBuff;    //!< Buffer where the received data go.

    Actions::Identify::Info                                   m_info;
    std::unordered_map<cmd_id_t, Actions::CommandInfo::Reply> m_commands;
    bool                                                      m_log = false;
    Frasy::Type::Manager                                      m_typeManager;

    std::unordered_map<trs_id_t, ResponsePromise> m_pending;
    void                                          GetDeviceCommands();
    void                                          GetDeviceEnums();
    void                                          GetDeviceStructs();

    static constexpr size_t s_maxAttempts = 5;
};
}    // namespace Frasy::Communication

#endif    // FRASY_UTILS_COMM_SERIAL_DEVICE_H
