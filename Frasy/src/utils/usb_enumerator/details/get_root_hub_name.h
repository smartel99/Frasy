/**
 * @file    get_root_hub_name.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_ROOT_HUB_NAME_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_ROOT_HUB_NAME_H

#include "oops.h"

#include <Brigerad/Utils/types/wstring_to_utf8.h>

#include <winnt.h>

#include <string>

namespace Frasy::Usb::Details {
inline std::string GetRootHubName(HANDLE handle)
{
    ULONG             nBytes = 0;
    USB_ROOT_HUB_NAME rootHubName;
    BOOL              success = DeviceIoControl(handle,
                                   IOCTL_USB_GET_ROOT_HUB_NAME,
                                   nullptr,
                                   0,
                                   &rootHubName,
                                   sizeof(rootHubName),
                                   &nBytes,
                                   nullptr);

    if (success == FALSE) {
        FRASY_USB_OOPS();
        return "";
    }

    // Allocate space to hold the root hub name.
    nBytes                          = rootHubName.ActualLength;
    PUSB_ROOT_HUB_NAME rootHubNameW = (PUSB_ROOT_HUB_NAME)malloc(nBytes);
    success                         = DeviceIoControl(handle,
                              IOCTL_USB_GET_ROOT_HUB_NAME,
                              nullptr,
                              0,
                              rootHubNameW,
                              nBytes,
                              &nBytes,
                              nullptr);
    auto wname = std::wstring(rootHubNameW->RootHubName);
    free(rootHubNameW);
    if (!success) {
        FRASY_USB_OOPS();
        return "";
    }

    return wstring_to_utf8(wname);
}
}

#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_ROOT_HUB_NAME_H
