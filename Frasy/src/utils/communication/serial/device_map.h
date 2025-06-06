/**
 * @file    device_map.h
 * @author  Samuel Martel
 * @date    2022-12-13
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

#ifndef FRASY_UTILS_COMMUNICATION_SERIAL_DEVICE_MAP_H
#define FRASY_UTILS_COMMUNICATION_SERIAL_DEVICE_MAP_H

#include "device.h"
#include "enumerator.h"

#include <atomic>
#include <map>

namespace Frasy::Serial
{
class DeviceMap
{
public:
    static DeviceMap& Get()
    {
        static DeviceMap instance;
        return instance;
    }

    bool isScanning();

    ResponsePromise& transmit(uint8_t deviceId, const Packet& pkt)
    {
        waitForScanComplete();
        return m_devices.at(deviceId).transmit(pkt);
    }

    Device& operator[](std::size_t index)
    {
        waitForScanComplete();
        return m_devices.at(static_cast<uint8_t>(index));
    }

    auto begin() noexcept
    {
        waitForScanComplete();
        return m_devices.begin();
    }
    auto end() noexcept
    {
        waitForScanComplete();
        return m_devices.end();
    }
    auto begin() const noexcept
    {
        waitForScanComplete();
        return m_devices.begin();
    }
    auto end() const noexcept
    {
        waitForScanComplete();
        return m_devices.end();
    }

    [[nodiscard]] size_t size() const noexcept
    {
        waitForScanComplete();
        return m_devices.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        waitForScanComplete();
        return m_devices.empty();
    }

private:
    void waitForScanComplete() const {
        throw std::exception(); // Device map shouldn't be used like this anymore.
        return m_scan_done.wait(false); }

private:
    std::map<uint8_t, Device> m_devices;
    std::future<size_t>             m_scan_future;
    std::atomic<bool>               m_scan_done = false;

    static constexpr const char* s_tag = "DeviceMap";
};
}    // namespace Frasy::Communication

#endif    // FRASY_UTILS_COMMUNICATION_SERIAL_DEVICE_MAP_H
