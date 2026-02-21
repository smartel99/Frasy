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
#include "response.h"

#include <Brigerad/Core/Log.h>
#include <Brigerad/Core/Time.h>
#include <functional>
#include <mutex>
#include <serial/serial.h>
#include <thread>
#include <vector>

namespace Frasy::Serial {
/**
 * Handles background reception of data from the serial device, as well as the transmission.
 *
 * Received packets are forwarded to the
 */
class Device {
public:
    Device() noexcept = default;
    Device(Device&& o) noexcept { *this = std::move(o); }
    explicit Device(const std::string& port, bool open = true);
    ~Device() { close(); }

    Device& operator=(Device&& o) noexcept;

    void open();
    void close();
    void reset();

    [[nodiscard]] bool        isOpen() const noexcept { return m_device.isOpen(); }
    [[nodiscard]] std::string getPort() const noexcept { return m_device.getPort(); }
    [[nodiscard]] bool        ready() const noexcept { return m_ready; }
    [[nodiscard]] bool        enabled() const noexcept { return m_enabled; }

    ResponsePromise& transmit(Packet pkt);

    [[nodiscard]] std::vector<trs_id_t> getPendingTransactions();

private:
    void checkForPackets();
    void cleanerTask();

private:
    std::string    m_label;
    serial::Serial m_device;    //!< The physical communication interface.

    std::jthread   m_cleanerThread;
    volatile bool m_cleanerRun = false;

    bool m_ready   = false;
    bool m_enabled = true;

    std::mutex    m_promiseLock;
    volatile bool m_shouldRun = true;
    std::jthread   m_rxThread;
    std::string   m_rxBuff;    //!< Buffer where the received data go.

    std::unordered_map<trs_id_t, ResponsePromise> m_pending;

    static constexpr size_t s_maxAttempts = 5;
};
}  // namespace Frasy::Serial

#endif    // FRASY_UTILS_COMM_SERIAL_DEVICE_H
