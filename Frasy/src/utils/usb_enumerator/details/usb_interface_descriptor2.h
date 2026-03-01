/**
 * @file    usb_interface_descriptor2.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_USB_INTERFACE_DESCRIPTOR2_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_USB_INTERFACE_DESCRIPTOR2_H

namespace Frasy::Usb::Details {
struct UsbInterfaceDescriptor2 {
    UCHAR  bLength            = {};    // offset 0, size 1
    UCHAR  bDescriptorType    = {};    // offset 1, size 1
    UCHAR  bInterfaceNumber   = {};    // offset 2, size 1
    UCHAR  bAlternateSetting  = {};    // offset 3, size 1
    UCHAR  bNumEndpoints      = {};    // offset 4, size 1
    UCHAR  bInterfaceClass    = {};    // offset 5, size 1
    UCHAR  bInterfaceSubClass = {};    // offset 6, size 1
    UCHAR  bInterfaceProtocol = {};    // offset 7, size 1
    UCHAR  iInterface         = {};    // offset 8, size 1
    USHORT wNumClasses        = {};    // offset 9, size 2
};
}    // namespace Frasy::Usb::Utils

#endif    // FRASY_UTILS_USB_ENUMERATOR_DETAILS_USB_INTERFACE_DESCRIPTOR2_H
