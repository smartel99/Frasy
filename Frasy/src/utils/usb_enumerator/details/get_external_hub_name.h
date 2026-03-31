/**
 * @file    get_external_hub_name.h
 * @author  Sam Martel
 * @date    2026-03-02
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_EXTERNAL_HUB_NAME_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_EXTERNAL_HUB_NAME_H

#include "oops.h"
#include "Brigerad/Utils/types/wstring_to_utf8.h"

#include <optional>
#include <string>

namespace Frasy::Usb::Details {
inline std::optional<std::string> GetExternalHubName(HANDLE device, uint8_t port)
{
    USB_NODE_CONNECTION_NAME extHubName;
    // Get the length of the name of the external hub attached to the specified port.
    extHubName.ConnectionIndex = port;
    ULONG nBytes               = 0;
    BOOL  success              = DeviceIoControl(device,
                                   IOCTL_USB_GET_NODE_CONNECTION_NAME,
                                   &extHubName,
                                   sizeof(extHubName),
                                   &extHubName,
                                   sizeof(extHubName),
                                   &nBytes,
                                   nullptr);
    if (success == FALSE || extHubName.ActualLength <= sizeof(extHubName)) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    // Allocate space to hold the external hub name.
    nBytes                                = extHubName.ActualLength;
    PUSB_NODE_CONNECTION_NAME extHubNameW = (PUSB_NODE_CONNECTION_NAME)malloc(nBytes);
    extHubNameW->ConnectionIndex          = port;
    success                               = DeviceIoControl(device,
                              IOCTL_USB_GET_NODE_CONNECTION_NAME,
                              extHubNameW,
                              nBytes,
                              extHubNameW,
                              nBytes,
                              &nBytes,
                              nullptr);
    auto wname = std::wstring(extHubNameW->NodeName);
    free(extHubNameW);
    if (success == FALSE) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }
    return wstring_to_utf8(wname);
}
}

#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_EXTERNAL_HUB_NAME_H
