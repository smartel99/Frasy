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

namespace Frasy::Communication
{
class DeviceMap
{
public:
    static DeviceMap& Get()
    {
        static DeviceMap instance;
        return instance;
    }

    std::future<size_t> ScanForDevices();

    const PrettyInstrumentationCardInfo& GetDeviceInfo(uint8_t deviceId) const
    {
        WaitForScanComplete();
        return m_devices.at(deviceId).GetInfo();
    }

    TransmissionCallbacks& Transmit(uint8_t deviceId, const Packet& pkt)
    {
        WaitForScanComplete();
        return m_devices.at(deviceId).Transmit(pkt);
    }

    auto begin() noexcept
    {
        WaitForScanComplete();
        return m_devices.begin();
    }
    auto end() noexcept
    {
        WaitForScanComplete();
        return m_devices.end();
    }
    auto begin() const noexcept
    {
        WaitForScanComplete();
        return m_devices.begin();
    }
    auto end() const noexcept
    {
        WaitForScanComplete();
        return m_devices.end();
    }

private:
    void WaitForScanComplete() const { return m_isScanning.wait(true); }

private:
    std::map<uint8_t, SerialDevice> m_devices;

    std::atomic_flag m_isScanning = ATOMIC_FLAG_INIT;

    static constexpr const char* s_tag = "DeviceMap";
};
}    // namespace Frasy::Communication

#endif    // FRASY_UTILS_COMMUNICATION_SERIAL_DEVICE_MAP_H
