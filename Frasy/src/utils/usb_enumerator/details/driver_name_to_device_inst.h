/**
 * @file    driver_name_to_device_inst.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_DRIVER_NAME_TO_DEVICE_INST_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_DRIVER_NAME_TO_DEVICE_INST_H

namespace Frasy::Usb::Details {
struct DeviceProps {
    HDEVINFO        deviceInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA deviceInfoData;
};

inline bool gRefreshCache = false;

inline std::optional<DeviceProps> DriverNameToDeviceInst(const std::string& driverName)
{
    DeviceProps deviceProps;
    memset(&deviceProps.deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));

    // We cannot walk the device tree with CM_Get_Sibling etc. unless we assume
    // the device tree will stabilize. Any devnode removal (even outside of USB)
    // would force us to retry. Instead, we use Setup API to snapshot all
    // devices.
    // TODO have a way to refresh the cache.
    // struct DeviceCacheEntry {
    //     HDEVINFO        deviceInfo;
    //     SP_DEVINFO_DATA deviceInfoData;
    //     std::string     driverName;
    // };
    // static std::vector<DeviceCacheEntry> deviceCache;
    // if (gRefreshCache) {
    //     BR_LOG_DEBUG("UsbEnum", "Refreshing device cache");
    //     gRefreshCache = false;
    //     deviceCache.clear();
    //
    //     // Examine all present devices to see if any match the given DriverName
    //     HDEVINFO deviceInfo = SetupDiGetClassDevsA(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    //     if (deviceInfo == INVALID_HANDLE_VALUE) {
    //         FRASY_USB_OOPS();
    //         return std::nullopt;
    //     }
    //
    //     ULONG           deviceIndex = 0;
    //     SP_DEVINFO_DATA deviceInfoData;
    //     memset(&deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
    //     deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    //
    //     BOOL done             = FALSE;
    //     while (done == FALSE) {
    //         // Get devinst of the next device.
    //         BOOL status = SetupDiEnumDeviceInfo(deviceInfo, deviceIndex, &deviceInfoData);
    //         deviceIndex++;
    //         if (status == FALSE) {
    //             // Could be an error, or indication that all devices have been processed. Either way, the desired device was not found.
    //             if (GetLastError() != ERROR_NO_MORE_ITEMS) { FRASY_USB_OOPS(); }
    //             done = TRUE;
    //             break;
    //         }
    //
    //         // Get the DriverName value
    //         auto maybeName = GetDeviceProperty(deviceInfo, &deviceInfoData, SPDRP_DRIVER);
    //         if (maybeName.has_value()) {
    //             deviceCache.emplace_back(deviceInfo, deviceInfoData, maybeName.value());
    //         }
    //     }
    //     std::ranges::sort(deviceCache, [](const auto& a, const auto& b) { return a.driverName < b.driverName; });
    // }
    //
    // auto it = std::ranges::find_if(deviceCache,
    //                                [&driverName](const DeviceCacheEntry& dce) { return dce.driverName == driverName; });
    // if (it != deviceCache.end()) {
    //     return DeviceProps{
    //         .deviceInfo = it->deviceInfo,
    //         .deviceInfoData = it->deviceInfoData};
    // }


    // Examine all present devices to see if any match the given DriverName
    HDEVINFO deviceInfo = SetupDiGetClassDevsA(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (deviceInfo == INVALID_HANDLE_VALUE) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    ULONG           deviceIndex = 0;
    SP_DEVINFO_DATA deviceInfoData;
    memset(&deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    BOOL done = FALSE;
    while (done == FALSE) {
        // Get devinst of the next device.
        BOOL status = SetupDiEnumDeviceInfo(deviceInfo, deviceIndex, &deviceInfoData);
        deviceIndex++;
        if (status == FALSE) {
            // Could be an error, or indication that all devices have been processed. Either way, the desired device was not found.
            if (GetLastError() != ERROR_NO_MORE_ITEMS) { FRASY_USB_OOPS(); }
            done = TRUE;
            break;
        }

        // Get the DriverName value
        auto maybeName = GetDeviceProperty(deviceInfo, &deviceInfoData, SPDRP_DRIVER);
        if (maybeName.has_value() && maybeName.value() == driverName) {
            return DeviceProps{deviceInfo, deviceInfoData};
        }
    }

    if (deviceInfo != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(deviceInfo);
    }
    BR_LOG_ERROR("UsbEnum", "Could not find device with driver name: {}", driverName);
    return std::nullopt;
}
}
#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_DRIVER_NAME_TO_DEVICE_INST_H
