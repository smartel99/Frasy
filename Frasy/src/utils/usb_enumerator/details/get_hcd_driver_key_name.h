/**
 * @file    get_hcd_driver_key_name.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_HCD_DRIVER_KEY_NAME_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_HCD_DRIVER_KEY_NAME_H
#include <windows.h>
#include <winioctl.h>
#include "oops.h"

#include <Brigerad/Utils/types/wstring_to_utf8.h>

#include <string>

#include <usbuser.h>
#include <usbioctl.h>

namespace Frasy::Usb::Details {
inline std::string GetHCDDriverKeyName(HANDLE hcd)
{
    USB_HCD_DRIVERKEY_NAME driverKeyName;
    ULONG                  nBytes = 0;
    ZeroMemory(&driverKeyName, sizeof(driverKeyName));

    // Get length of the name of the driver key of the HCD.
    bool success = DeviceIoControl(hcd,
                                   IOCTL_GET_HCD_DRIVERKEY_NAME,
                                   &driverKeyName,
                                   sizeof(driverKeyName),
                                   &driverKeyName,
                                   sizeof(driverKeyName),
                                   &nBytes,
                                   nullptr);
    if (!success) {
        FRASY_USB_OOPS();
        return "";
    }

    nBytes                                 = driverKeyName.ActualLength;
    PUSB_HCD_DRIVERKEY_NAME driverKeyNameW = (PUSB_HCD_DRIVERKEY_NAME)malloc(nBytes);
    // Get the name of the driver key of the device attached to the specified port.
    success = DeviceIoControl(hcd,
                              IOCTL_GET_HCD_DRIVERKEY_NAME,
                              driverKeyNameW,
                              nBytes,
                              driverKeyNameW,
                              nBytes,
                              &nBytes,
                              nullptr);
    auto wName = std::wstring(driverKeyNameW->DriverKeyName);
    free(driverKeyNameW);
    if (!success) {
        FRASY_USB_OOPS();
        return "";
    }

    return wstring_to_utf8(wName);
}
}

#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_HCD_DRIVER_KEY_NAME_H
