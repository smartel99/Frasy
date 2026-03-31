/**
 * @file    pnp_strings.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_PNP_STRINGS_H
#define FRASY_UTILS_USB_ENUMERATOR_PNP_STRINGS_H

#include <string>

namespace Frasy::Usb {
struct DevicePnpStrings {
    std::string deviceId;
    std::string deviceDesc;
    std::string hwId;
    std::string service;
    std::string deviceClass;
    std::string powerState;
    std::string friendlyName;

    void setDevicePnpStrings(const DevicePnpStrings& other)
    {
        deviceId = other.deviceId;
        deviceDesc = other.deviceDesc;
        hwId = other.hwId;
        service = other.service;
        deviceClass = other.deviceClass;
        powerState = other.powerState;
        friendlyName = other.friendlyName;
    }
};
}
#endif //FRASY_UTILS_USB_ENUMERATOR_PNP_STRINGS_H
