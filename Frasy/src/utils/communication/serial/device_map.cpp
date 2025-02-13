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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "device_map.h"

#include "enumerator.h"

#include <Brigerad/Core/Log.h>
#include <future>

namespace Frasy::Serial
{

bool DeviceMap::isScanning()
{
    using namespace std::chrono_literals;
    return m_scan_future.valid() && m_scan_future.wait_for(50us) == std::future_status::timeout;
}
}    // namespace Frasy::Communication
