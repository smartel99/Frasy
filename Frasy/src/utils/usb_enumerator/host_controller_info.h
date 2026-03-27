/**
 * @file    host_controller_info.h
 * @author  Sam Martel
 * @date    2026-02-26
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_HOST_CONTROLLER_INFO_H
#define FRASY_UTILS_USB_ENUMERATOR_HOST_CONTROLLER_INFO_H

#include "usb_node.h"
#include "pnp_strings.h"
#include "root_hub_info.h"

#include <vector>
#include <variant>
#include <cstddef>
#include <array>
#include <string>

#include <windows.h>
#include <usb.h>
#include <usbuser.h>

namespace Frasy::Usb {

struct HostControllerInfo : DevicePnpStrings {
    RootHubInfo rootHub;
    std::string driverKey;
    size_t vendorId = 0;
    size_t deviceId = 0;
    size_t subSysId = 0;
    size_t revision = 0;
    std::array<USB_POWER_INFO,6> USBPowerInfo;
    bool busDeviceFunctionValid;
    size_t busNumber = 0;
    size_t busDevice = 0;
    size_t busFunction = 0;
    USB_CONTROLLER_INFO_0 controllerInfo;
};
}

#endif //FRASY_UTILS_USB_ENUMERATOR_HOST_CONTROLLER_INFO_H
