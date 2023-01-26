/**
 * @file    device_map.cpp
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
#include "device_map.h"

#include "enumerator.h"

#include <Brigerad/Core/Log.h>
#include <future>

namespace Frasy::Communication
{
std::future<size_t> DeviceMap::ScanForDevices()
{
    return std::async(std::launch::async,
                      [this]()
                      {
                          m_isScanning.test_and_set();

                          BR_APP_DEBUG("Closing {} serial devices...", m_devices.size());
                          m_devices.clear();

                          BR_APP_INFO("Scanning for instrumentation cards...");

                          std::vector<Communication::DeviceInfo> devices =
                            Communication::EnumerateInstrumentationCards();
                          BR_APP_INFO("{} devices found!", devices.size());

                          for (auto&& device : devices) { m_devices[device.Info.Id] = SerialDevice(device); }

                          m_isScanning.clear();
                          m_isScanning.notify_all();

                          return m_devices.size();
                      });
}
}    // namespace Frasy::Communication
