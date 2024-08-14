/**
 * @file    device.cpp
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
#include "device.h"

#include <Brigerad.h>
#include <utils/string_utils.h>

namespace Frasy::SlCan {
Device::Device(std::string_view port, bool open)
: m_label("SlCan"),
  m_device(std::string {port},
           921600,
           serial::Timeout::simpleTimeout(10),
           serial::eightbits,
           serial::parity_none,
           serial::stopbits_one,
           serial::flowcontrol_software)
{
    if (open) { this->open(); }
}

Device& Device::operator=(Device&& o) noexcept
{
    bool reopen = false;
    if (o.m_device.isOpen()) {
        reopen = true;
        o.close();
    }

    m_label = std::move(o.m_label);
    m_queue = std::move(o.m_queue);
    m_muted = o.m_muted.load();

    try {
        serial::Timeout timeout = serial::Timeout::simpleTimeout(10);
        m_device.setTimeout(timeout);
        m_device.setBaudrate(921'600);
        m_device.setPort(o.m_device.getPort());
        if (reopen) { open(); }
    }
    catch (std::exception& e) {
        BR_LOG_ERROR(m_label, "Unable to configure port '{}': {}", o.m_device.getPort(), e.what());
    }
    return *this;
}

size_t Device::transmit(const Packet& pkt)
{
    BR_LOG_DEBUG(m_label, "Sending {} packet", commandToStr(pkt.command));

    uint8_t buff[Packet::s_mtu] = {};
    auto    size                = pkt.toSerial(&buff[0], sizeof(buff));
    if (size != -1) {
        try {
            auto written = m_device.write(&buff[0], size);
            m_device.flushOutput();
            m_txMonitorFunc(pkt);
            return written;
        }
        catch (std::exception& e) {
            BR_LOG_ERROR(m_label, "While transmitting on '{}': {}", m_device.getPort(), e.what());
            return 0;
        }
    }
    return 0;
}

Packet Device::receive()
{
    std::unique_lock lk {m_lock};
    m_cv.wait(lk, [this] { return !m_muted && !m_queue.empty(); });

    auto front = m_queue.front();
    m_queue.pop();
    return front;
}

void Device::open()
{
    // If already open, re-open.
    if (m_device.isOpen()) { close(); }

    try {
        m_device.open();
    }
    catch (std::exception& e) {
        BR_LOG_ERROR(m_label, "While opening '{}': {}", m_device.getPort(), e.what());
        return;
    }

    m_rxThread = std::jthread([&](std::stop_token stopToken) {
        if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)) {
            BR_LOG_ERROR("SlCAN", "Unable to set thread priority!");
        }
        BR_LOG_INFO(m_label, "Started RX listener on '{}'", m_device.getPort());
        std::string read;
        while (!stopToken.stop_requested()) {
            try {
                read.clear();
                [[maybe_unused]] size_t len = m_device.readline(read, Packet::s_mtu, "\r");
                if (m_muted || read.empty()) { continue; }
                BR_LOG_TRACE(m_label, "RX: {}", read);
                std::unique_lock lock {m_lock};
                const auto&      packet = m_queue.emplace(reinterpret_cast<const uint8_t*>(read.data()), read.size());
                m_rxMonitorFunc(packet);
                // manual unlocking is done before notifying, to avoid waking up
                // the waiting thread only to block again (see notify_one for details)
                lock.unlock();
                m_cv.notify_one();
            }
            catch (std::exception& e) {
                if (!stopToken.stop_requested()) {
                    auto getLastError = [] {
                        char buff[100] = {};
                        auto len       = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                                  nullptr,
                                                  GetLastError(),
                                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                                  &buff[0],
                                                  sizeof(buff),
                                                  nullptr);
                        return std::string {&buff[0], &buff[len]};
                    };
                    BR_LOG_ERROR(
                      m_label, "An error occurred in the listener thread: {}\n\r\t{}", e.what(), getLastError());
                }
                break;
            }
        }
        BR_LOG_INFO(m_label, "RX listener terminated on '{}'", m_device.getPort());
    });
}

void Device::close()
{
    // If already closed, don't do anything.
    if (m_device.isOpen()) {
        // Forcefully terminate *any* I/O operation done by the thread.
        if (CancelSynchronousIo(m_rxThread.native_handle()) == 0) {
            BR_LOG_WARN(m_label, "CancelSynchronousIo returned {}", GetLastError());
        }
        m_device.close();
    }
}
}    // namespace Frasy::SlCan
