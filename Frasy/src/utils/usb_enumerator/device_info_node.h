/**
 * @file    device_info_node.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DEVICE_INFO_NODE_H
#define FRASY_UTILS_USB_ENUMERATOR_DEVICE_INFO_NODE_H
#include "usb_node.h"

#include <string>
#include <vector>

#include <windows.h>
#include <setupapi.h>

namespace Frasy::Usb {
struct DeviceInfoNode {
    HDEVINFO                         deviceInfo;
    std::vector<Node>                nodes;
    SP_DEVINFO_DATA                  deviceInfoData;
    SP_DEVICE_INTERFACE_DATA         deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData;
    std::string                      deviceDescName;
    std::string                      deviceDriverName;
    DEVICE_POWER_STATE               latestDevicePowerState;
};
} // namespace Frasy::Usb

#endif //FRASY_UTILS_USB_ENUMERATOR_DEVICE_INFO_NODE_H
