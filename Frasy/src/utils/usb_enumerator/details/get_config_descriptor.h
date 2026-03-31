/**
 * @file    get_config_descriptor.h
 * @author  Sam Martel
 * @date    2026-03-01
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_CONFIG_DESCRIPTOR_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_CONFIG_DESCRIPTOR_H

#include "oops.h"
#include "usb_descriptor_request.h"

#include <memory>
#include <optional>

#include <usbioctl.h>

namespace Frasy::Usb::Details {
inline std::optional<UsbDescriptorRequest> GetConfigDescriptor(HANDLE  handle,
                                                               uint8_t configIndex,
                                                               uint8_t descriptorIndex)
{
    uint8_t configDescReqBuf[sizeof(USB_DESCRIPTOR_REQUEST) + sizeof(USB_CONFIGURATION_DESCRIPTOR)];
    // Do some cursed things to avoid dynamically allocating stuff! :D
    PUSB_DESCRIPTOR_REQUEST       configDescReq = reinterpret_cast<PUSB_DESCRIPTOR_REQUEST>(configDescReqBuf);
    PUSB_CONFIGURATION_DESCRIPTOR configDesc =
      reinterpret_cast<PUSB_CONFIGURATION_DESCRIPTOR>(configDescReqBuf + sizeof(USB_DESCRIPTOR_REQUEST));

    ULONG nBytes = sizeof(configDescReqBuf);
    // Zero-fill everything
    memset(configDescReqBuf, 0, nBytes);

    // Indicate the port from which the descriptor will be requested
    configDescReq->ConnectionIndex = configIndex;

    // USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
    // IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
    //
    // USBD will automatically initialize these fields:
    //     bmRequest = 0x80
    //     bRequest  = 0x06
    //
    // We must inititialize these fields:
    //     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
    //     wIndex    = Zero (or Language ID for String Descriptors)
    //     wLength   = Length of descriptor buffer
    configDescReq->SetupPacket.wValue  = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8) | descriptorIndex;
    configDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

    ULONG nBytesReturned = 0;
    // Now issue the get descriptor request.
    BOOL success = DeviceIoControl(handle,
                                   IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
                                   configDescReq,
                                   nBytes,
                                   configDescReq,
                                   nBytes,
                                   &nBytesReturned,
                                   nullptr);

    // TODO Doesn't really make sense to check configDesc when we have done nothing with it yet?
    if (success == FALSE || nBytesReturned != nBytes ||
        configDesc->wTotalLength < sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    // Now request the entire Configuration Descriptor using a dynamically allocated buffer which is sized big enough to
    // hold the entire descriptor.
    nBytes = sizeof(USB_DESCRIPTOR_REQUEST) + configDesc->wTotalLength;
    std::unique_ptr<uint8_t[]> configDescBuf(new uint8_t[nBytes]);
    configDescReq = reinterpret_cast<PUSB_DESCRIPTOR_REQUEST>(configDescBuf.get());
    configDesc = reinterpret_cast<PUSB_CONFIGURATION_DESCRIPTOR>(configDescBuf.get() + sizeof(USB_DESCRIPTOR_REQUEST));

    // Re-indicate the port from which the descriptor will be requested.
    configDescReq->ConnectionIndex     = configIndex;
    configDescReq->SetupPacket.wValue  = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8) | descriptorIndex;
    configDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

    success = DeviceIoControl(handle,
                              IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
                              configDescReq,
                              nBytes,
                              configDescReq,
                              nBytes,
                              &nBytesReturned,
                              nullptr);
    if (success == FALSE || nBytesReturned != nBytes ||
        configDesc->wTotalLength < sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    return UsbDescriptorRequest {configDescReq};
}
}    // namespace Frasy::Usb::Details

#endif    // FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_CONFIG_DESCRIPTOR_H
