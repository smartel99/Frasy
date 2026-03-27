/**
 * @file    root_hub_info.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_ROOT_HUB_INFO_H
#define FRASY_UTILS_USB_ENUMERATOR_ROOT_HUB_INFO_H

#include "pnp_strings.h"
#include "device_info_node.h"

#include "details/usb_port_connector_properties.h"

#include <string>

#include <windows.h>
#include <usbioctl.h>

namespace Frasy::Usb {
struct RootHubInfo : public DevicePnpStrings, DeviceInfoNode {
    USB_NODE_INFORMATION       hubInfo;
    USB_HUB_INFORMATION_EX     hubInfoEx;
    std::string                hubName;
    UsbPortConnectorProperties portConnectorProperties;
    USB_HUB_CAPABILITIES_EX    hubCapabilitiesEx;
};
}

#endif //FRASY_UTILS_USB_ENUMERATOR_ROOT_HUB_INFO_H
