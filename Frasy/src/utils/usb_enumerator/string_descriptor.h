/**
 * @file    string_descriptor.h
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_STRING_DESCRIPTOR_H
#define FRASY_UTILS_USB_ENUMERATOR_STRING_DESCRIPTOR_H

#include <Brigerad/Utils/types/wstring_to_utf8.h>

#include <cstdint>
#include <string>

#include <windows.h>
#include <usbspec.h>

namespace Frasy::Usb {
struct StringDescriptor {
    StringDescriptor()                                   = default;
    StringDescriptor(const StringDescriptor&)            = default;
    StringDescriptor(StringDescriptor&&)                 = default;
    StringDescriptor& operator=(const StringDescriptor&) = default;
    StringDescriptor& operator=(StringDescriptor&&)      = default;
    ~StringDescriptor()                                  = default;

    StringDescriptor(const USB_STRING_DESCRIPTOR* desc)
        : length(desc->bLength),
          descriptorType(desc->bDescriptorType),
          string(desc->bString)
    {
        toUtf8();
    }

    const std::string& toUtf8() const { return utf8; }

    const std::string& toUtf8()
    {
        if (utf8.empty() && !string.empty()) {
            utf8 = wstring_to_utf8(string);
        }
        return utf8;
    }

    uint8_t      length         = 0;
    uint8_t      descriptorType = 0;
    std::wstring string;

private:
    std::string utf8;
};

struct StringDescriptorNode {
    uint8_t          descriptorIndex = 0;
    USHORT           languageId      = 0;
    StringDescriptor stringDescriptor;
};
}

#endif //FRASY_UTILS_USB_ENUMERATOR_STRING_DESCRIPTOR_H
