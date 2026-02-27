/**
 * @file    usb_connection_status.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_FORMAT_USB_CONNECTION_STATUS_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_FORMAT_USB_CONNECTION_STATUS_H

#include <string_view>

#include <usbioctl.h>

namespace Frasy::Usb {
constexpr std::string_view toString(USB_CONNECTION_STATUS status)
{
    switch (status) {
        case DeviceFailedEnumeration: return "FailedEnumeration";
        case DeviceGeneralFailure: return "GeneralFailure";
        case DeviceCausedOvercurrent: return "CausedOvercurrent";
        case DeviceNotEnoughPower: return "NotEnoughPower";
        case DeviceNotEnoughBandwidth: return "NotEnoughBandwidth";
        case DeviceHubNestedTooDeeply: return "HubNestedTooDeeply";
        case DeviceInLegacyHub: return "InLegacyHub";
        case DeviceEnumerating: return "Enumerating";
        case DeviceReset: return "Reset";
        case NoDeviceConnected:
        case DeviceConnected:
        default: return "";
    }
}
}

#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_FORMAT_USB_CONNECTION_STATUS_H
