/**
 * @file    get_string_descriptor.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_STRING_DESCRIPTOR_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_STRING_DESCRIPTOR_H

#include "../string_descriptor.h"
#include "oops.h"

#include <cstdint>
#include <optional>

namespace Frasy::Usb::Details {
inline std::optional<StringDescriptorNode> GetStringDescriptor(HANDLE   device,
                                                               ULONG    connIndex,
                                                               uint8_t  descIndex,
                                                               uint16_t languageId)
{
    alignas(USB_DESCRIPTOR_REQUEST) uint8_t stringDescReqBuff[
        sizeof(USB_DESCRIPTOR_REQUEST) + MAXIMUM_USB_STRING_LENGTH];
    auto* stringDescReq = reinterpret_cast<PUSB_DESCRIPTOR_REQUEST>(stringDescReqBuff);
    auto* stringDesc    = reinterpret_cast<PUSB_STRING_DESCRIPTOR>(stringDescReqBuff + sizeof(USB_DESCRIPTOR_REQUEST));
    ULONG nBytes        = sizeof(stringDescReqBuff);
    // zero-fill the entire request structure.
    memset(stringDescReqBuff, 0, nBytes);

    // Indicate the port from which the descriptor will be requested.
    stringDescReq->ConnectionIndex = connIndex;

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
    stringDescReq->SetupPacket.wValue  = (USB_STRING_DESCRIPTOR_TYPE << 8) | descIndex;
    stringDescReq->SetupPacket.wIndex  = languageId;
    stringDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

    // Now issue the get descriptor request.
    ULONG nBytesReturned = 0;
    BOOL  success        = DeviceIoControl(device,
                                   IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
                                   stringDescReq,
                                   nBytes,
                                   stringDescReq,
                                   nBytes,
                                   &nBytesReturned,
                                   nullptr);

    // Do some sanity checks on the return from the get descriptor request.
    if (success == FALSE) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    if (nBytesReturned < 2) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    if (stringDesc->bDescriptorType != USB_STRING_DESCRIPTOR_TYPE) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    if (stringDesc->bLength != nBytesReturned - sizeof(USB_DESCRIPTOR_REQUEST)) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    if (stringDesc->bLength % 2 != 0) {
        FRASY_USB_OOPS();
        return std::nullopt;
    }

    // All is good! Convert to the output.
    return StringDescriptorNode{
        .descriptorIndex = descIndex,
        .languageId = languageId,
        .stringDescriptor = stringDesc,};
}
} // namespace Frasy::Usb::Details

#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_STRING_DESCRIPTOR_H
