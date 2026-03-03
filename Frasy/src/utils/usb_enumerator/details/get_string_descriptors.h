/**
 * @file    get_string_descriptors.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_STRING_DESCRIPTORS_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_STRING_DESCRIPTORS_H

#include "string_descriptor.h"
#include "get_string_descriptor.h"

#include <vector>

namespace Frasy::Usb::Details {
inline std::vector<StringDescriptorNode> GetStringDescriptors(HANDLE device, ULONG port, uint8_t descIndex, const std::wstring& languageIds)
{
    // TODO Windows has a cache here that it reads from
    // https://github.com/microsoft/Windows-driver-samples/blob/main/usb/usbview/enum.c#L2703

    // Get the next string descriptor. If empty, then we're done.
    std::vector<StringDescriptorNode> stringDescriptors;
    stringDescriptors.reserve(languageIds.size());
    for (wchar_t languageId : languageIds) {
        auto maybeStringDescriptor = GetStringDescriptor(device, port, descIndex, languageId);
        if (!maybeStringDescriptor.has_value()) { break; }
        stringDescriptors.push_back(maybeStringDescriptor.value());
    }

    return stringDescriptors;
}
}

#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_GET_STRING_DESCRIPTORS_H
