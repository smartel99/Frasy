/**
 * @file    usb_enumerator.cpp
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
#include "usb_enumerator.h"
#include "details/get_hcd_driver_key_name.h"
#include "details/driver_name_to_device_properties.h"
#include "details/get_host_controller_info.h"
#include "details/oops.h"
#include "details/get_root_hub_name.h"
#include "details/enumerate_hub.h"

#include <Brigerad/Core/Log.h>

#include <initguid.h>

#include <windows.h>
#include <windowsx.h>
#include <devioctl.h>
#include <dbt.h>
#include <stdio.h>
#include <stddef.h>
#include <commctrl.h>
#include <usbioctl.h>
#include <usbiodef.h>
#include <intsafe.h>
#include <strsafe.h>
#include <specstrings.h>
#include <usb.h>
#include <usbuser.h>
#include <basetyps.h>
#include <wtypes.h>
#include <objbase.h>
#include <io.h>
#include <conio.h>
#include <shellapi.h>
#include <cfgmgr32.h>
#include <shlwapi.h>
#include <setupapi.h>
#include <winioctl.h>
#include <devpkey.h>
#include <math.h>

namespace Frasy::Usb {
namespace {


std::optional<Node> EnumerateUsbHostController(HANDLE           deviceHandle,
                                               const char*      deviceName,
                                               const HDEVINFO&  deviceInfo,
                                               SP_DEVINFO_DATA* deviceInfoData)
{
    HostControllerInfo hcInfo;

    hcInfo.driverKey = Details::GetHCDDriverKeyName(deviceHandle);
    if (hcInfo.driverKey.empty()) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }
    BR_LOG_TRACE("UsbEnum", "Found Driver Key Name: {}", hcInfo.driverKey);

    // TODO Windows example skips the host if it sees it was already enumerated.

    if (hcInfo.driverKey.size() < 256) {
        auto maybeStrings = Details::DriverNameToDeviceProperties(hcInfo.driverKey);
        if (maybeStrings.has_value()) {
            hcInfo.setDevicePnpStrings(*maybeStrings);
            BR_LOG_TRACE("UsbEnum", "Driver desc: {}", hcInfo.deviceDesc);
        }
        else {
            BR_LOG_ERROR("UsbEnum", "Could not get device PNP strings for driver name: {}", hcInfo.driverKey);
        }
    }

    // Get the USB Host Controller power map
    // TODO

    // Get bus, device and function
    hcInfo.busDeviceFunctionValid = false;
    BOOL success                  = SetupDiGetDeviceRegistryPropertyA(deviceInfo,
                                                     deviceInfoData,
                                                     SPDRP_BUSNUMBER,
                                                     nullptr,
                                                     (PBYTE)&hcInfo.busNumber,
                                                     sizeof(hcInfo.busNumber),
                                                     nullptr);
    ULONG deviceAndFunction = 0;
    if (success == TRUE) {
        success = SetupDiGetDeviceRegistryPropertyA(deviceInfo,
                                                    deviceInfoData,
                                                    SPDRP_ADDRESS,
                                                    nullptr,
                                                    (PBYTE)&deviceAndFunction,
                                                    sizeof(deviceAndFunction),
                                                    nullptr);
    }

    if (success == TRUE) {
        hcInfo.busDevice              = deviceAndFunction >> 16;
        hcInfo.busFunction            = deviceAndFunction & 0xFFFF;
        hcInfo.busDeviceFunctionValid = true;
    }

    if (!Details::GetHostControllerInfo(deviceHandle, hcInfo)) {
        FRASY_USB_OOPS();
    }

    std::string rootHubName = Details::GetRootHubName(deviceHandle);
    BR_LOG_TRACE("UsbEnum", "Root Hub Name: {}", rootHubName);
    if (rootHubName.empty()) {
        FRASY_USB_OOPS();
    }
    else {
        hcInfo.rootHub = std::get<0>(Details::EnumerateHub(rootHubName));
    }

    return hcInfo;
}

std::vector<Node> EnumerateUsbHostControllers()
{
    std::vector<Node> controllers;

    // https://learn.microsoft.com/en-us/windows-hardware/drivers/install/guid-devinterface-usb-host-controller
    GUID     usbHostControllerGuid = GUID{0x3ABF6F2D, 0x71C4, 0x462A, {0x8A, 0x92, 0x1E, 0x68, 0x61, 0xE6, 0xAF, 0x27}};
    HDEVINFO deviceInfo            = SetupDiGetClassDevs(&usbHostControllerGuid,
                                              nullptr,
                                              nullptr,
                                              (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    SP_DEVINFO_DATA deviceInfoData;

    deviceInfoData.cbSize = sizeof(deviceInfoData);

    for (size_t index = 0; SetupDiEnumDeviceInfo(deviceInfo, index, &deviceInfoData) != 0; ++index) {
        SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
        deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);

        for (int devInterfaceIndex = 0; SetupDiEnumDeviceInterfaces(deviceInfo,
                                                                    &deviceInfoData,
                                                                    &usbHostControllerGuid,
                                                                    devInterfaceIndex,
                                                                    &deviceInterfaceData) != 0; ++devInterfaceIndex) {
            ULONG requiredLength = 0;
            BOOL  success        = SetupDiGetDeviceInterfaceDetail(deviceInfo,
                                                           &deviceInterfaceData,
                                                           nullptr,
                                                           0,
                                                           &requiredLength,
                                                           nullptr);
            if (!success && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                FRASY_USB_OOPS();
                break;
            }

            PSP_DEVICE_INTERFACE_DETAIL_DATA deviceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)
                malloc(requiredLength);
            if (deviceDetailData == nullptr) {
                FRASY_USB_OOPS();
                break;
            }
            deviceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            success                  = SetupDiGetDeviceInterfaceDetail(deviceInfo,
                                                      &deviceInterfaceData,
                                                      deviceDetailData,
                                                      requiredLength,
                                                      &requiredLength,
                                                      nullptr);
            if (!success) {
                FRASY_USB_OOPS();
            }

            HANDLE deviceHandle = CreateFile(deviceDetailData->DevicePath,
                                             GENERIC_WRITE,
                                             FILE_SHARE_WRITE,
                                             nullptr,
                                             OPEN_EXISTING,
                                             0,
                                             nullptr);

            // If handle is valid, then we've successfully opened a Host Controller.
            if (deviceHandle != INVALID_HANDLE_VALUE) {
                auto maybeNode = EnumerateUsbHostController(deviceHandle,
                                                            deviceDetailData->DevicePath,
                                                            deviceInfo,
                                                            &deviceInfoData);
                if (maybeNode.has_value()) { controllers.emplace_back(*maybeNode); }
                CloseHandle(deviceHandle);
            }

            free(deviceDetailData);
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfo);

    BR_CORE_DEBUG("Enumerated {} USB Host Controllers", controllers.size());
    return controllers;
}
}


std::vector<Node> EnumerateUsbTree()
{
    Details::gRefreshCache = true;
    return EnumerateUsbHostControllers();
}
}
