/**
 * @file    usb_device_info.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_USB_DEVICE_INFO_H
#define FRASY_UTILS_USB_ENUMERATOR_USB_DEVICE_INFO_H

#include "pnp_strings.h"
#include "device_info_node.h"
#include "string_descriptor.h"
#include "details/usb_descriptor_request.h"
#include "details/usb_node_connection_information_ex.h"
#include "details/usb_port_connector_properties.h"

#include <optional>
#include <string>

namespace Frasy::Usb {
struct DeviceInfo : DeviceInfoNode, DevicePnpStrings {
    std::optional<USB_NODE_INFORMATION> hubInfo; // Only if HUB
    std::optional<USB_HUB_INFORMATION_EX> hubInfoEx; // Only if HUB
    std::optional<std::string> hubName; // Only if HUB
    std::optional<UsbNodeConnectionInformationEx> connectionInfo;  // Not present if root hub
    UsbPortConnectorProperties portConnectorProps;
    std::optional<UsbDescriptorRequest> configDesc;  // Not present if root hub
    std::optional<UsbDescriptorRequest> bosDesc; // Not present if root hub
    std::vector<StringDescriptor> stringDescriptors;
    USB_NODE_CONNECTION_INFORMATION_EX_V2 connectionInfoV2; // Not present if root hub
    std::optional<USB_HUB_CAPABILITIES_EX> hubCapabilitiesEx; // Only if not a hub.
};
}

#endif //FRASY_UTILS_USB_ENUMERATOR_USB_DEVICE_INFO_H
