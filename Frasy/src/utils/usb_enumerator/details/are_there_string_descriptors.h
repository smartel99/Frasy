/**
 * @file    are_there_string_descriptors.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_ARE_THERE_STRING_DESCRIPTORS_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_ARE_THERE_STRING_DESCRIPTORS_H

#include "oops.h"
#include "usb_interface_descriptor2.h"

#include <usbspec.h>

namespace Frasy::Usb::Details {
inline bool AreThereStringDescriptors(const USB_DEVICE_DESCRIPTOR*        deviceDesc,
                                      const USB_CONFIGURATION_DESCRIPTOR* configDesc)
{
    // Check device descriptor strings.
    if (deviceDesc->iManufacturer != 0 || deviceDesc->iProduct != 0 || deviceDesc->iSerialNumber != 0) { return true; }

    // Check the configuration and interface descriptor strings.
    const UCHAR* descEnd    = reinterpret_cast<const UCHAR*>(configDesc + configDesc->wTotalLength);
    const auto*  commonDesc = reinterpret_cast<const USB_COMMON_DESCRIPTOR*>(configDesc);
    while ((const PUCHAR)commonDesc + sizeof(USB_COMMON_DESCRIPTOR) < descEnd &&
           (const PUCHAR)commonDesc + commonDesc->bLength <= descEnd) {
        switch (commonDesc->bDescriptorType) {
            case USB_CONFIGURATION_DESCRIPTOR_TYPE:
            case USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_TYPE:
                if (commonDesc->bLength != sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
                    FRASY_USB_OOPS();
                    break;
                }
                if (((PUSB_CONFIGURATION_DESCRIPTOR)commonDesc)->iConfiguration != 0) { return true; }
                commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
                continue;

            case USB_INTERFACE_DESCRIPTOR_TYPE:
                if (commonDesc->bLength != sizeof(USB_INTERFACE_DESCRIPTOR) &&
                    commonDesc->bLength != sizeof(UsbInterfaceDescriptor2)) {
                    FRASY_USB_OOPS();
                    break;
                }
                if (((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->iInterface != 0) { return true; }
                commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
                continue;
            default: commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength); continue;
        }
    }

    return false;
}
}    // namespace Frasy::Usb::Details

#endif    // FRASY_UTILS_USB_ENUMERATOR_DETAILS_ARE_THERE_STRING_DESCRIPTORS_H
