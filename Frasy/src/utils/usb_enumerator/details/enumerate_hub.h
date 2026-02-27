/**
 * @file    enumerate_hub.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_ENUMERATE_HUB_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_ENUMERATE_HUB_H
#include "../root_hub_info.h"
#include "../pnp_strings.h"
#include "../string_descriptor.h"
#include "../usb_device_info.h"
#include "enumerate_hub_ports.h"
#include "format/format.h"

#include "utils/misc/visit.h"

#include <string>
#include <vector>
#include <variant>
#include <format>
#include <optional>

namespace Frasy::Usb::Details {
inline std::variant<RootHubInfo, ExternalHubInfo> EnumerateHub(
    const std::string&                     hubName,
    PUSB_NODE_CONNECTION_INFORMATION_EX    connectionInfo          = nullptr,
    PUSB_NODE_CONNECTION_INFORMATION_EX_V2 connectionInfoV2        = nullptr,
    PUSB_PORT_CONNECTOR_PROPERTIES         portConnectorProperties = nullptr,
    PUSB_DESCRIPTOR_REQUEST                configDesc              = nullptr,
    PUSB_DESCRIPTOR_REQUEST                bosDesc                 = nullptr,
    const std::vector<StringDescriptor>&   stringDescriptors       = {},
    std::optional<DevicePnpStrings>        devicePnpStrings        = std::nullopt)
{
    std::variant<RootHubInfo, ExternalHubInfo> devInfo;
    USB_NODE_INFORMATION                       hubInfo;
    USB_HUB_INFORMATION_EX                     hubInfoEx;
    USB_HUB_CAPABILITIES_EX                    hubCapabilitiesEx;

    if (connectionInfo != nullptr) { devInfo = ExternalHubInfo{}; }
    else { devInfo = RootHubInfo{}; }
    Frasy::visit(devInfo,
                 [&](ExternalHubInfo& info) {
                     info.hubName            = hubName;
                     info.connectionInfo     = connectionInfo;
                     info.configDesc         = configDesc;
                     info.stringDescriptors  = stringDescriptors;
                     info.portConnectorProps = portConnectorProperties;
                     info.hubInfoEx          = hubInfoEx;
                     info.hubCapabilitiesEx  = hubCapabilitiesEx;
                     info.bosDesc            = bosDesc;
                     info.connectionInfoV2   = *connectionInfoV2;
                     if (devicePnpStrings.has_value()) { info.setDevicePnpStrings(*devicePnpStrings); }
                 },
                 [&](RootHubInfo& info) {
                     info.hubName                 = hubName;
                     info.hubInfoEx               = hubInfoEx;
                     info.hubCapabilitiesEx       = hubCapabilitiesEx;
                     info.portConnectorProperties = portConnectorProperties;
                     if (devicePnpStrings.has_value()) { info.setDevicePnpStrings(*devicePnpStrings); }
                 });

    // Buffer for the full hub device name.
    auto deviceName = std::format("\\\\.\\{}", hubName);
    // Try to open the hub device.
    auto hubDevice = CreateFile(deviceName.c_str(),
                                GENERIC_WRITE,
                                FILE_SHARE_WRITE,
                                nullptr,
                                OPEN_EXISTING,
                                0,
                                nullptr);
    if (hubDevice == INVALID_HANDLE_VALUE) {
        FRASY_USB_OOPS();
        return {};
    }

    // Now query USBHUB for the USB_NODE_INFORMATION structure for this hub.
    // This will tell us the number of downstream ports to enumerate, among
    // other things.
    DWORD nBytes  = 0;
    BOOL  success = DeviceIoControl(hubDevice,
                                   IOCTL_USB_GET_NODE_INFORMATION,
                                   &hubInfo,
                                   sizeof(USB_NODE_INFORMATION),
                                   &hubInfo,
                                   sizeof(USB_NODE_INFORMATION),
                                   &nBytes,
                                   nullptr);
    if (success == FALSE) {
        FRASY_USB_OOPS();
        return {};
    }

    success = DeviceIoControl(hubDevice,
                              IOCTL_USB_GET_HUB_INFORMATION_EX,
                              &hubInfoEx,
                              sizeof(USB_HUB_INFORMATION_EX),
                              &hubInfoEx,
                              sizeof(USB_HUB_INFORMATION_EX),
                              &nBytes,
                              nullptr);

    // Fail gracefully.
    if (success == TRUE) {
        Frasy::visit(devInfo,
                     [&](RootHubInfo& info) { info.hubInfoEx = hubInfoEx; },
                     [&](ExternalHubInfo& info) { info.hubInfoEx = hubInfoEx; });
    }

    // Obtain Hub Capabilities
    success = DeviceIoControl(hubDevice,
                              IOCTL_USB_GET_HUB_CAPABILITIES_EX,
                              &hubCapabilitiesEx,
                              sizeof(USB_HUB_CAPABILITIES_EX),
                              &hubCapabilitiesEx,
                              sizeof(USB_HUB_CAPABILITIES_EX),
                              &nBytes,
                              nullptr);

    if (success == TRUE) {
        Frasy::visit(devInfo, [&](auto& info) { info.hubCapabilitiesEx = hubCapabilitiesEx; });
    }

    // Build the leaf name from the port number and the device description.
    std::string leafName;
    if (connectionInfo != nullptr) {
        leafName = std::format("[Port{}]{} :  ", connectionInfo->ConnectionIndex, connectionInfo->ConnectionStatus);
    }
    if (devicePnpStrings.has_value()) {
        leafName += devicePnpStrings.value().deviceDesc;
    }
    else {
        Frasy::visit(devInfo,
                     [&](RootHubInfo&) { leafName += "RootHub"; },
                     [&](ExternalHubInfo&) { leafName += hubName; });
    }

    // TODO is this where this goes? lol
    Frasy::visit(devInfo, [&](auto&& info) { info.deviceDescName = leafName; });

    // Recursively list the ports of this hub.
    Frasy::visit(devInfo,
                 [&](auto& info) {
                     info.nodes = EnumerateHubPorts(hubDevice, hubInfo.u.HubInformation.HubDescriptor.bNumberOfPorts);
                 });

    CloseHandle(hubDevice);

    return devInfo;
}
}  // namespace Frasy::Usb::Details
#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_ENUMERATE_HUB_H
