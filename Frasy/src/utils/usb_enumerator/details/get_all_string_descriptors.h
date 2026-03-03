/**
 * @file    get_all_string_descriptors.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_ALL_STRING_DESCRIPTORS_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_ALL_STRING_DESCRIPTORS_H

#include "get_string_descriptor.h"
#include "get_string_descriptors.h"
#include "string_descriptor.h"

#include <optional>
#include <string>
#include <vector>

namespace Frasy::Usb::Details {
inline std::optional<std::vector<StringDescriptorNode>> GetAllStringDescriptors(
  HANDLE device, uint8_t port, const USB_DEVICE_DESCRIPTOR* devDesc, const USB_CONFIGURATION_DESCRIPTOR* confDesc)
{
    auto stringDescriptors = std::vector<StringDescriptorNode>(1);    // index 0 is supported languages.

    // Get the array of supported language IDs, which is returned in String Descriptor 0.
    auto maybeLanguages = GetStringDescriptor(device, port, 0, 0);
    if (!maybeLanguages.has_value()) { return std::nullopt; }
    stringDescriptors[0] = maybeLanguages.value();

    // Get them strings.
    if (devDesc->iManufacturer != 0) {
        stringDescriptors.append_range(
          GetStringDescriptors(device, port, devDesc->iManufacturer, stringDescriptors[0].stringDescriptor.string));
    }
    if (devDesc->iProduct != 0) {
        stringDescriptors.append_range(
          GetStringDescriptors(device, port, devDesc->iProduct, stringDescriptors[0].stringDescriptor.string));
    }
    if (devDesc->iSerialNumber != 0) {
        stringDescriptors.append_range(
          GetStringDescriptors(device, port, devDesc->iSerialNumber, stringDescriptors[0].stringDescriptor.string));
    }

    // Get the configuration and interface descriptor strings.
    const uint8_t* descEnd    = reinterpret_cast<const uint8_t*>(confDesc + confDesc->wTotalLength);
    const auto*    commonDesc = reinterpret_cast<const USB_COMMON_DESCRIPTOR*>(confDesc);

    while ((const uint8_t*)commonDesc + sizeof(USB_COMMON_DESCRIPTOR) < descEnd &&
           (const uint8_t*)commonDesc + commonDesc->bLength <= descEnd) {
        if (commonDesc->bLength == 0) { break; }
        switch (commonDesc->bDescriptorType) {
            // case USB_ENDPOINT_DESCRIPTOR_TYPE: break;    // TODO might want to implement that.
            case USB_CONFIGURATION_DESCRIPTOR_TYPE: {
                if (commonDesc->bLength != sizeof(USB_CONFIGURATION_DESCRIPTOR)) {
                    FRASY_USB_OOPS();
                    break;
                }
                if (((PUSB_CONFIGURATION_DESCRIPTOR)commonDesc)->iConfiguration != 0) {
                    stringDescriptors.append_range(
                      GetStringDescriptors(device,
                                           port,
                                           ((PUSB_CONFIGURATION_DESCRIPTOR)commonDesc)->iConfiguration,
                                           stringDescriptors[0].stringDescriptor.string));
                }
                commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
                continue;
            }
            case USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE: {
                if (commonDesc->bLength < sizeof(USB_INTERFACE_ASSOCIATION_DESCRIPTOR)) {
                    FRASY_USB_OOPS();
                    break;
                }
                if (((PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR)commonDesc)->iFunction != 0) {
                    stringDescriptors.append_range(
                      GetStringDescriptors(device,
                                           port,
                                           ((PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR)commonDesc)->iFunction,
                                           stringDescriptors[0].stringDescriptor.string));
                }
                commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
                continue;
            }
            case USB_INTERFACE_DESCRIPTOR_TYPE: {
                if (commonDesc->bLength != sizeof(USB_INTERFACE_DESCRIPTOR) &&
                    commonDesc->bLength != sizeof(UsbInterfaceDescriptor2)) {
                    FRASY_USB_OOPS();
                    break;
                }
                if (((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->iInterface != 0) {
                    stringDescriptors.append_range(
                      GetStringDescriptors(device,
                                           port,
                                           ((PUSB_INTERFACE_DESCRIPTOR)commonDesc)->iInterface,
                                           stringDescriptors[0].stringDescriptor.string));
                }
                commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
                continue;
            }
            default:
                BR_LOG_DEBUG("UsbEnum", "Skipping unknown descriptor type: {:02x}", commonDesc->bDescriptorType);
                commonDesc = (PUSB_COMMON_DESCRIPTOR)((PUCHAR)commonDesc + commonDesc->bLength);
                continue;
        }
    }

    return stringDescriptors;
}
}    // namespace Frasy::Usb::Details

#endif    // FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_ALL_STRING_DESCRIPTORS_H
