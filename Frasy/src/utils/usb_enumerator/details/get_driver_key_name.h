/**
 * @file    get_driver_key_name.h
 * @author  Sam Martel
 * @date    2026-03-01
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_GET_DRIVER_KEY_NAME_H
#define FRASY_UTILS_USB_ENUMERATOR_GET_DRIVER_KEY_NAME_H

#include "oops.h"

#include "Brigerad/Utils/types/wstring_to_utf8.h"

#include <optional>
#include <string>

namespace Frasy::Usb {
inline std::optional<std::string> GetDriverKeyName(HANDLE device, uint8_t index)
{
    USB_NODE_CONNECTION_DRIVERKEY_NAME driverKeyName;
    ULONG                              nBytes = 0;
    // Get the length of the name of the driver key of the device attached to the specified port.
    driverKeyName.ConnectionIndex = index;
    BOOL success                  = DeviceIoControl(device,
                                   IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
                                   &driverKeyName,
                                   sizeof(driverKeyName),
                                   &driverKeyName,
                                   sizeof(driverKeyName),
                                   &nBytes,
                                   nullptr);
    if (!success) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    nBytes = driverKeyName.ActualLength;
    if (nBytes <= sizeof(driverKeyName)) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }
    PUSB_NODE_CONNECTION_DRIVERKEY_NAME driverKeyNameW = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)malloc(nBytes);
    driverKeyNameW->ActualLength                       = nBytes;
    success                                            = DeviceIoControl(device,
                              IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
                              driverKeyNameW,
                              nBytes,
                              driverKeyNameW,
                              nBytes,
                              &nBytes,
                              nullptr);
    std::wstring wname                                 = std::wstring(driverKeyNameW->DriverKeyName);
    free(driverKeyNameW);
    if (!success) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    return wstring_to_utf8(wname);
}
}    // namespace Frasy::Usb

#endif    // FRASY_UTILS_USB_ENUMERATOR_GET_DRIVER_KEY_NAME_H
