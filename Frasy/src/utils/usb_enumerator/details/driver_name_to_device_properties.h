/**
 * @file    driver_name_to_device_properties.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DRIVER_NAME_TO_DEVICE_PROPERTIES_H
#define FRASY_UTILS_USB_ENUMERATOR_DRIVER_NAME_TO_DEVICE_PROPERTIES_H

#include "../pnp_strings.h"
#include "driver_name_to_device_inst.h"
#include "get_device_property.h"

#include <optional>

#include <SetupAPI.h>

namespace Frasy::Usb::Details {
inline std::optional<DevicePnpStrings> DriverNameToDeviceProperties(const std::string& driverName)
{
    auto maybeInstance = DriverNameToDeviceInst(driverName);
    if (!maybeInstance) { return std::nullopt; }
    auto& [deviceInfo, deviceInfoData] = maybeInstance.value();

    ULONG requiredLen = 0;
    BOOL  status      = SetupDiGetDeviceInstanceIdA(deviceInfo, &deviceInfoData, nullptr, 0, &requiredLen);
    LONG  lastError   = GetLastError();
    if (status != FALSE && lastError != ERROR_INSUFFICIENT_BUFFER) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    // An extra byte is required for the terminating character
    requiredLen++;
    DevicePnpStrings devicePnpStrings;
    devicePnpStrings.deviceId = std::string(requiredLen, '\0');
    status                    = SetupDiGetDeviceInstanceIdA(
      deviceInfo, &deviceInfoData, (PSTR)devicePnpStrings.deviceId.data(), requiredLen, &requiredLen);
    if (status == FALSE) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    auto maybeDesc = GetDeviceProperty(deviceInfo, &deviceInfoData, SPDRP_DEVICEDESC);
    if (maybeDesc.has_value()) { devicePnpStrings.deviceDesc = maybeDesc.value(); }
    else {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    // We don't fail if the following registry query fails as these fields are additional information only
    auto maybeProp = GetDeviceProperty(deviceInfo, &deviceInfoData, SPDRP_HARDWAREID);
    if (maybeProp.has_value()) { devicePnpStrings.hwId = maybeProp.value(); }
    maybeProp = GetDeviceProperty(deviceInfo, &deviceInfoData, SPDRP_SERVICE);
    if (maybeProp.has_value()) { devicePnpStrings.service = maybeProp.value(); }
    maybeProp = GetDeviceProperty(deviceInfo, &deviceInfoData, SPDRP_CLASS);
    if (maybeProp.has_value()) { devicePnpStrings.deviceClass = maybeProp.value(); }
    maybeProp = GetDeviceProperty(deviceInfo, &deviceInfoData, SPDRP_FRIENDLYNAME);
    if (maybeProp.has_value()) { devicePnpStrings.friendlyName = maybeProp.value(); }

    return devicePnpStrings;
}
}    // namespace Frasy::Usb::Details

#endif    // FRASY_UTILS_USB_ENUMERATOR_DRIVER_NAME_TO_DEVICE_PROPERTIES_H
