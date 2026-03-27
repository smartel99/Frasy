/**
 * @file    usb_descriptor_request.h
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


#ifndef STRATO_USB_DESCRIPTOR_REQUEST_H
#define STRATO_USB_DESCRIPTOR_REQUEST_H
#include <string>
#include <cstdint>

#include <usbioctl.h>

namespace Frasy::Usb {
struct UsbDescriptorRequest {
    UsbDescriptorRequest()                                           = default;
    UsbDescriptorRequest(const UsbDescriptorRequest&)                = default;
    UsbDescriptorRequest(UsbDescriptorRequest&&) noexcept            = default;
    UsbDescriptorRequest& operator=(const UsbDescriptorRequest&)     = default;
    UsbDescriptorRequest& operator=(UsbDescriptorRequest&&) noexcept = default;
    ~UsbDescriptorRequest()                                          = default;

    UsbDescriptorRequest(const PUSB_DESCRIPTOR_REQUEST pO)
    {
        if (pO == nullptr) { return; }
        ConnectionIndex       = pO->ConnectionIndex;
        SetupPacket.bmRequest = pO->SetupPacket.bmRequest;
        SetupPacket.bRequest  = pO->SetupPacket.bRequest;
        SetupPacket.wValue    = pO->SetupPacket.wValue;
        SetupPacket.wIndex    = pO->SetupPacket.wIndex;
        SetupPacket.wLength   = pO->SetupPacket.wLength;
        Data.assign(pO->Data, pO->Data + pO->SetupPacket.wLength);
    }

    uint32_t ConnectionIndex = 0;

    struct {
        uint8_t  bmRequest = 0;
        uint8_t  bRequest  = 0;
        uint16_t wValue    = 0;
        uint16_t wIndex    = 0;
        uint16_t wLength   = 0;
    } SetupPacket;

    std::string Data;
};
}
#endif //STRATO_USB_DESCRIPTOR_REQUEST_H
