/**
 * @file    get_device_property.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_GET_DEVICE_PROPERTY_H
#define FRASY_UTILS_USB_ENUMERATOR_GET_DEVICE_PROPERTY_H

#include "oops.h"

#include <optional>
#include <string>

#include <SetupAPI.h>

namespace Frasy::Usb::Details {
inline std::optional<std::string> GetDeviceProperty(HDEVINFO         deviceInfoSet,
                                                    PSP_DEVINFO_DATA deviceInfoData,
                                                    DWORD            propertyId)
{
    DWORD requiredLen = 0;
    BOOL  res         = SetupDiGetDeviceRegistryPropertyA(deviceInfoSet,
                                                         deviceInfoData,
                                                         propertyId,
                                                         nullptr,
                                                         nullptr,
                                                         0,
                                                         &requiredLen);
    DWORD lastError = GetLastError();
    if ((requiredLen == 0) && (res != FALSE && lastError != ERROR_INSUFFICIENT_BUFFER)) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    auto info = std::string(requiredLen, '\0');
    res       = SetupDiGetDeviceRegistryPropertyA(deviceInfoSet,
                                                 deviceInfoData,
                                                 propertyId,
                                                 nullptr,
                                                 (PBYTE)info.data(),
                                                 requiredLen,
                                                 &requiredLen);
    if (res == FALSE) {
        return std::nullopt;
    }
    // Remove the extra \0
    info.resize(requiredLen - 1);
    return info;
}
}

#endif //FRASY_UTILS_USB_ENUMERATOR_GET_DEVICE_PROPERTY_H
