/**
 * @file    get_bos_descriptor.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_BOS_DESCRIPTOR_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_BOS_DESCRIPTOR_H

#include "oops.h"
#include "usb_descriptor_request.h"

#include <optional>
#include <memory>

namespace Frasy::Usb::Details {
inline std::optional<UsbDescriptorRequest> GetBosDescriptor(HANDLE handle, uint8_t index)
{
    uint8_t bosDescReqBuf[sizeof(USB_DESCRIPTOR_REQUEST) + sizeof(USB_BOS_DESCRIPTOR)];
    // Do some cursed things to avoid dynamically allocating stuff! :D
    PUSB_DESCRIPTOR_REQUEST       bosDescReq = reinterpret_cast<PUSB_DESCRIPTOR_REQUEST>(bosDescReqBuf);
    PUSB_BOS_DESCRIPTOR bosDesc =
      reinterpret_cast<PUSB_BOS_DESCRIPTOR>(bosDescReqBuf + sizeof(USB_DESCRIPTOR_REQUEST));

    ULONG nBytes = sizeof(bosDescReqBuf);
    // Zero-fill everything
    memset(bosDescReqBuf, 0, nBytes);

    // Indicate the port from which the descriptor will be requested
    bosDescReq->ConnectionIndex = index;

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
    bosDescReq->SetupPacket.wValue  = (USB_BOS_DESCRIPTOR_TYPE << 8);
    bosDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

    ULONG nBytesReturned = 0;
    // Now issue the get descriptor request.
    BOOL success = DeviceIoControl(handle,
                                   IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
                                   bosDescReq,
                                   nBytes,
                                   bosDescReq,
                                   nBytes,
                                   &nBytesReturned,
                                   nullptr);

    // TODO Doesn't really make sense to check bosDesc when we have done nothing with it yet?
    if (success == FALSE || nBytesReturned != nBytes ||
        bosDesc->wTotalLength < sizeof(USB_BOS_DESCRIPTOR)) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    // Now request the entire Configuration Descriptor using a dynamically allocated buffer which is sized big enough to
    // hold the entire descriptor.
    nBytes = sizeof(USB_DESCRIPTOR_REQUEST) + bosDesc->wTotalLength;
    std::unique_ptr<uint8_t[]> bosDescBuf(new uint8_t[nBytes]);
    bosDescReq = reinterpret_cast<PUSB_DESCRIPTOR_REQUEST>(bosDescBuf.get());
    bosDesc = reinterpret_cast<PUSB_BOS_DESCRIPTOR>(bosDescBuf.get() + sizeof(USB_DESCRIPTOR_REQUEST));

    // Re-indicate the port from which the descriptor will be requested.
    bosDescReq->ConnectionIndex     = index;
    bosDescReq->SetupPacket.wValue  = (USB_BOS_DESCRIPTOR_TYPE << 8);
    bosDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

    success = DeviceIoControl(handle,
                              IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
                              bosDescReq,
                              nBytes,
                              bosDescReq,
                              nBytes,
                              &nBytesReturned,
                              nullptr);
    if (success == FALSE || nBytesReturned != nBytes ||
        bosDesc->wTotalLength < sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    return UsbDescriptorRequest {bosDescReq};
}
}

#endif    // FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_BOS_DESCRIPTOR_H
