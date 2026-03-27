/**
 * @file    usb_node_connection_information_ex.h
 * @author  Sam Martel
 * @date    2026-02-27
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_USB_NODE_CONNECTION_INFORMATION_EX_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_USB_NODE_CONNECTION_INFORMATION_EX_H

#include <vector>

namespace Frasy::Usb {
struct UsbNodeConnectionInformationEx {
    UsbNodeConnectionInformationEx()                                                     = default;
    UsbNodeConnectionInformationEx(const UsbNodeConnectionInformationEx&)                = default;
    UsbNodeConnectionInformationEx(UsbNodeConnectionInformationEx&&) noexcept            = default;
    UsbNodeConnectionInformationEx& operator=(const UsbNodeConnectionInformationEx&)     = default;
    UsbNodeConnectionInformationEx& operator=(UsbNodeConnectionInformationEx&&) noexcept = default;
    ~UsbNodeConnectionInformationEx()                                                    = default;

    UsbNodeConnectionInformationEx(PUSB_NODE_CONNECTION_INFORMATION_EX pInfo)
    {
        if (pInfo == nullptr) { return; }
        ConnectionIndex           = pInfo->ConnectionIndex;
        DeviceDescriptor          = pInfo->DeviceDescriptor;
        CurrentConfigurationValue = pInfo->CurrentConfigurationValue;
        Speed                     = pInfo->Speed;
        DeviceIsHub               = pInfo->DeviceIsHub;
        DeviceAddress             = pInfo->DeviceAddress;
        NumberOfOpenPipes         = pInfo->NumberOfOpenPipes;
        ConnectionStatus          = pInfo->ConnectionStatus;
        PipeList.clear();
        PipeList.reserve(pInfo->NumberOfOpenPipes);
        for (size_t i = 0; i < pInfo->NumberOfOpenPipes; i++) {
            PipeList.emplace_back(pInfo->PipeList[i]);
        }
    }

    ULONG ConnectionIndex = 0; /* INPUT */
    /* usb device descriptor returned by this device
       during enumeration */
    USB_DEVICE_DESCRIPTOR DeviceDescriptor = {};          /* OUTPUT */
    UCHAR                 CurrentConfigurationValue = 0; /* OUTPUT */
    /* values for the speed field are defined in USB200.h */
    UCHAR                      Speed = 0;             /* OUTPUT */
    BOOLEAN                    DeviceIsHub = FALSE;       /* OUTPUT */
    USHORT                     DeviceAddress = 0;     /* OUTPUT */
    ULONG                      NumberOfOpenPipes = 0; /* OUTPUT */
    USB_CONNECTION_STATUS      ConnectionStatus = DeviceReset;  /* OUTPUT */
    std::vector<USB_PIPE_INFO> PipeList;          /* OUTPUT */
};
}

#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_USB_NODE_CONNECTION_INFORMATION_EX_H
