/**
 * @file    enumerate_hub_ports.h
 * @author  Sam Martel
 * @date    2026-02-27
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_ENUMERATE_HUB_PORTS_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_ENUMERATE_HUB_PORTS_H

#include "../usb_node.h"

#include "Brigerad/Core/Log.h"

#include <vector>
#include <optional>
#include <memory>

namespace Frasy::Usb::Details {
inline std::optional<Node> EnumerateHubPort(HANDLE device, uint8_t port)
{
    ULONG nBytesEx = 0;
    ULONG nBytes   = 0;
    // Allocate space to hold the connection info for this port.
    // For now, allocate it big enough to hold info for 30 pipes.
    //
    // Endpoint numbers are 0-15.  Endpoint number 0 is the standard
    // control endpoint which is not explicitly listed in the Configuration
    // Descriptor.  There can be an IN endpoint and an OUT endpoint at
    // endpoint numbers 1-15 so there can be a maximum of 30 endpoints
    // per device configuration.
    static constexpr size_t pipeCount = 32;
    static constexpr size_t conInfoExBuffSize = sizeof(USB_NODE_CONNECTION_INFORMATION_EX) + (
                                                    sizeof(USB_PIPE_INFO) * pipeCount);
    alignas(USB_NODE_CONNECTION_INFORMATION_EX) char conInfoExBuff[conInfoExBuffSize];
    auto connectionInfoEx = reinterpret_cast<USB_NODE_CONNECTION_INFORMATION_EX*>(conInfoExBuff);
    USB_NODE_CONNECTION_INFORMATION_EX_V2 connectionInfoV2 = {
        .ConnectionIndex = port,
        .Length = sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2),
    };
    connectionInfoV2.SupportedUsbProtocols.Usb110 = 1;
    connectionInfoV2.SupportedUsbProtocols.Usb200 = 1;
    connectionInfoV2.SupportedUsbProtocols.Usb300 = 1;

    // Now query USBHUB for the structures for this port.  This will tell us if a device is attached to this
    // port, among other things. The fault tolerant code is executed first.
    USB_PORT_CONNECTOR_PROPERTIES  pcpSizeGetter = {.ConnectionIndex = port};
    std::unique_ptr<char[]>        pcpBuff;
    USB_PORT_CONNECTOR_PROPERTIES* portConnectorProperties = nullptr;
    BOOL                           success                 = DeviceIoControl(device,
                                   IOCTL_USB_GET_PORT_CONNECTOR_PROPERTIES,
                                   &pcpSizeGetter,
                                   sizeof(USB_PORT_CONNECTOR_PROPERTIES),
                                   &pcpSizeGetter,
                                   sizeof(USB_PORT_CONNECTOR_PROPERTIES),
                                   &nBytes,
                                   nullptr);

    // if (success == TRUE && nBytes == sizeof(USB_PORT_CONNECTOR_PROPERTIES)) {
    //     pcpBuff = std::unique_ptr<char[]>(
    //         new(static_cast<std::align_val_t>(alignof(USB_PORT_CONNECTOR_PROPERTIES))) char[pcpSizeGetter.
    //             ActualLength]);
    //     portConnectorProperties = reinterpret_cast<USB_PORT_CONNECTOR_PROPERTIES*>(pcpBuff.get());
    //     success                 = DeviceIoControl(device,
    //                               IOCTL_USB_GET_PORT_CONNECTOR_PROPERTIES,
    //                               portConnectorProperties,
    //                               pcpSizeGetter.ActualLength,
    //                               portConnectorProperties,
    //                               pcpSizeGetter.ActualLength,
    //                               &nBytes,
    //                               nullptr);
    //     if (success != TRUE || nBytes != pcpSizeGetter.ActualLength) {
    //         BR_LOG_ERROR("UsbEnum", "Failed to get port connector properties for port {}", port);
    //         portConnectorProperties = nullptr;
    //     }
    // }
    //
    // success = DeviceIoControl(device,
    //                           IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX_V2,
    //                           &connectionInfoV2,
    //                           sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2),
    //                           &connectionInfoV2,
    //                           sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2),
    //                           &nBytes,
    //                           nullptr);
    // if (success != TRUE || nBytes != sizeof(USB_NODE_CONNECTION_INFORMATION_EX_V2)) {
    //     BR_LOG_ERROR("UsbEnum", "Failed to get node connection information for port {}", port);
    //     connectionInfoV2 = {};
    // }
    //
    // connectionInfoEx->ConnectionIndex = port;
    // success                           = DeviceIoControl(device,
    //                           IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
    //                           connectionInfoEx,
    //                           conInfoExBuffSize,
    //                           connectionInfoEx,
    //                           conInfoExBuffSize,
    //                           &nBytesEx,
    //                           nullptr);
    // if (success == TRUE) {
    //     // Since the USB_NODE_CONNECTION_INFORMATION_EX is used to display
    //     // the device speed, but the hub driver doesn't support indication
    //     // of superspeed, we overwrite the value if the super speed
    //     // data structures are available and indicate the device is operating
    //     // at SuperSpeed.
    //     if (connectionInfoEx->Speed == UsbHighSpeed
    //         && (connectionInfoV2.Flags.DeviceIsOperatingAtSuperSpeedOrHigher ||
    //             connectionInfoV2.Flags.DeviceIsOperatingAtSuperSpeedPlusOrHigher)) {
    //         connectionInfoEx->Speed = UsbSuperSpeed;
    //     }
    // }
    // else {
    //     // Try to use IOCTL_USB_GET_NODE_CONNECTION_INFORMATION instead of _EX
    //     alignas(USB_NODE_CONNECTION_INFORMATION_EX) char conInfoBuff[conInfoExBuffSize];
    //     auto connectionInfo = reinterpret_cast<USB_NODE_CONNECTION_INFORMATION*>(conInfoBuff);
    //     connectionInfo->ConnectionIndex = port;
    //     success = DeviceIoControl(device,
    //                               IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
    //                               connectionInfo,
    //                               conInfoExBuffSize,
    //                               connectionInfo,
    //                               conInfoExBuffSize,
    //                               &nBytes,
    //                               nullptr);
    //     if (success != TRUE) {
    //         FRASY_USB_OOPS();
    //         return std::nullopt;
    //     }
    //
    //     // Copy the information to the var we'll be using.
    //     connectionInfoEx->ConnectionIndex = connectionInfo->ConnectionIndex;
    //     connectionInfoEx->DeviceDescriptor = connectionInfo->DeviceDescriptor;
    //     connectionInfoEx->CurrentConfigurationValue = connectionInfo->CurrentConfigurationValue;
    //     connectionInfoEx->Speed = connectionInfo->LowSpeed == TRUE ? UsbLowSpeed : UsbFullSpeed;
    //     connectionInfoEx->DeviceIsHub = connectionInfo->DeviceIsHub;
    //     connectionInfoEx->DeviceAddress = connectionInfo->DeviceAddress;
    //     connectionInfoEx->NumberOfOpenPipes = connectionInfo->NumberOfOpenPipes;
    //     connectionInfoEx->ConnectionStatus = connectionInfo->ConnectionStatus;
    //
    //     std::memcpy(connectionInfoEx->PipeList, connectionInfo->PipeList, sizeof(USB_PIPE_INFO) * pipeCount);
    // }

    return std::nullopt;
}

inline std::vector<Node> EnumerateHubPorts(HANDLE hub, uint8_t numPorts)
{
    std::vector<Node> ports;
    ports.reserve(numPorts);

    // Loop over all ports of the hub.
    // Port indices are 1-based, not 0-based.
    for (uint8_t index = 1; index <= numPorts; index++) {
        auto maybeNode = EnumerateHubPort(hub, index);
        if (maybeNode.has_value()) { ports.push_back(maybeNode.value()); }
        else { BR_LOG_ERROR("UsbEnum", "Failed to enumerate hub port {}", index); }
    }

    return ports;
}
}

#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_ENUMERATE_HUB_PORTS_H
