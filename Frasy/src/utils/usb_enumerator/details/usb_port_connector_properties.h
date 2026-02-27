/**
 * @file    usb_port_connector_properties.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_USB_PORT_CONNECTOR_PROPERTIES_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_USB_PORT_CONNECTOR_PROPERTIES_H
#include <string>

#include <usbioctl.h>

namespace Frasy::Usb {
struct UsbPortConnectorProperties {
    UsbPortConnectorProperties()                                                 = default;
    UsbPortConnectorProperties(const UsbPortConnectorProperties&)                = default;
    UsbPortConnectorProperties(UsbPortConnectorProperties&&) noexcept            = default;
    UsbPortConnectorProperties& operator=(const UsbPortConnectorProperties&)     = default;
    UsbPortConnectorProperties& operator=(UsbPortConnectorProperties&&) noexcept = default;
    ~UsbPortConnectorProperties()                                                = default;

    UsbPortConnectorProperties(const PUSB_PORT_CONNECTOR_PROPERTIES pO)
    {
        if (pO == nullptr) { return; }
        ConnectionIndex     = pO->ConnectionIndex;
        ActualLength        = pO->ActualLength;
        UsbPortProperties   = pO->UsbPortProperties;
        CompanionIndex      = pO->CompanionIndex;
        CompanionPortNumber = pO->CompanionPortNumber;
        if (CompanionPortNumber != 0) {
            // String only present if CompanionPortNumber is not zero.
            CompanionHubSymbolicLinkName = pO->CompanionHubSymbolicLinkName;
        }
    }

    // one based port number
    ULONG ConnectionIndex = 0;

    // The number of bytes required to hold the entire USB_PORT_CONNECTOR_PROPERTIES
    // structure, including the full CompanionHubSymbolicLinkName string
    ULONG ActualLength = 0;

    // bitmask of flags indicating properties and capabilities of the port
    USB_PORT_PROPERTIES UsbPortProperties = {};

    // Zero based index number of the companion port being queried.
    USHORT CompanionIndex = 0;

    // Port number of the companion port
    USHORT CompanionPortNumber = 0;

    // Symbolic link name for the companion hub
    std::wstring CompanionHubSymbolicLinkName;
};
}

#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_USB_PORT_CONNECTOR_PROPERTIES_H
