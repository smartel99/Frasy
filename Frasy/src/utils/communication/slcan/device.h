/**
 * @file    device.h
 * @author  Samuel Martel
 * @date    2024-04-16
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


#ifndef FRASY_SRC_UTILS_COMMUNICATION_SLCAN_DEVICE_H
#define FRASY_SRC_UTILS_COMMUNICATION_SLCAN_DEVICE_H
#include "packet.h"

#include <serial/serial.h>

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <string_view>

namespace Frasy {
class DeviceViewer;
}

namespace Frasy::SlCan {
class Device {
public:
             Device() noexcept = default;
             Device(Device&& o) noexcept { *this = std::move(o); }
             Device(const Device&) = delete;
    explicit Device(std::string_view port, bool open = true);
    ~        Device() { close(); }

    Device& operator=(Device&& o) noexcept;
    Device& operator=(const Device&) = delete;

    void open();
    void close();

    [[nodiscard]] bool        isOpen() const noexcept { return m_device.isOpen(); }
    [[nodiscard]] std::string getPort() const noexcept { return m_device.getPort(); }

    /**
     * Effectively mutes the reception of CAN messages on the port.
     *
     * In practice, this just results in every single packets not being queued.
     */
    void mute() { m_muted = true; }
    void unmute() { m_muted = false; }

    /**
     *
     * @param pkt Packet to be sent.
     * @return Number of bytes written, 0 on error.
     */
    size_t transmit(const Packet& pkt);
    Packet receive();

    [[nodiscard]] size_t available() const { return m_queue.size(); }

    void setRxCallbackFunc(const std::function<void()>& func)
    {
        if (func) { m_rxCallbackFunc = func; }
    }

private:
    std::string    m_label;
    serial::Serial m_device;    //!< The physical communication interface.

    std::jthread m_rxThread;

    std::mutex              m_lock;
    std::condition_variable m_cv;
    std::queue<Packet>      m_queue;

    std::atomic_bool m_muted = false;

    // Things used by the device viewer for monitoring purposes.
    friend class DeviceViewer;
    std::function<void(const Packet&)> m_rxMonitorFunc  = [](const Packet&) {};
    std::function<void(const Packet&)> m_txMonitorFunc  = [](const Packet&) {};
    std::function<void()>              m_rxCallbackFunc = [] {};
};
}    // namespace Frasy::SlCan

#endif    // FRASY_SRC_UTILS_COMMUNICATION_SLCAN_DEVICE_H
