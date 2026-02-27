/**
 * @file    get_host_controller_info.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_HOST_CONTROLLER_INFO_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_HOST_CONTROLLER_INFO_H

#include "../host_controller_info.h"
#include "oops.h"

#include <optional>

namespace Frasy::Usb::Details {
inline bool GetHostControllerInfo(HANDLE handle, HostControllerInfo& info)
{
    USBUSER_CONTROLLER_INFO_0 usbControllerInfo;
    memset(&usbControllerInfo, 0, sizeof(usbControllerInfo));
    // Set the header and request sizes.
    usbControllerInfo.Header.UsbUserRequest      = USBUSER_GET_CONTROLLER_INFO_0;
    usbControllerInfo.Header.RequestBufferLength = sizeof(usbControllerInfo);

    // Query for the USB_CONTROLLER_INFO_0 structure.
    DWORD dwBytes = 0;
    BOOL success = DeviceIoControl(handle,
                                   IOCTL_USB_USER_REQUEST,
                                   &usbControllerInfo,
                                   sizeof(usbControllerInfo),
                                   &usbControllerInfo,
                                   sizeof(usbControllerInfo),
                                   &dwBytes,
                                   nullptr);
    if (success == FALSE) {
        FRASY_USB_OOPS();
        return false;
    }

    info.controllerInfo = usbControllerInfo.Info0;

    return true;
}
}

#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_HOST_CONTROLLER_INFO_H
