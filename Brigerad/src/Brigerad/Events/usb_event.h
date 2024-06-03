/**
 * @file    usb_event.h
 * @author  Samuel Martel
 * @date    2024-04-15
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
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */


#ifndef BRIGERAD_SRC_BRIGERAD_EVENTS_USB_EVENT_H
#define BRIGERAD_SRC_BRIGERAD_EVENTS_USB_EVENT_H

#include "Brigerad/Utils/types/wstring_to_utf8.h"
#include "Event.h"

#include <codecvt>

namespace Brigerad {
class UsbEvent : public Event {
public:
    EVENT_CLASS_CATEGORY(EventCategoryUsb)
    std::wstring guid;
    std::wstring name;

protected:
    UsbEvent(const std::wstring& guid, const std::wstring& name) : guid(guid), name(name) {}
};

class BRIGERAD_API UsbConnectedEvent : public UsbEvent {
public:
    UsbConnectedEvent(const std::wstring& guid, const std::wstring& name) : UsbEvent(guid, name) {}

    std::string ToString() const override
    {
        std::wstringstream ss;
        ss << "UsbConnectedEvent {guid: " << guid << ", name: " << name << "}";
        return wstring_to_utf8(ss.str());
    }

    EVENT_CLASS_TYPE(EventType::UsbConnected)
};

class BRIGERAD_API UsbDisconnectedEvent : public UsbEvent {
public:
    UsbDisconnectedEvent(const std::wstring& guid, const std::wstring& name) : UsbEvent(guid, name) {}

    std::string ToString() const override
    {
        std::wstringstream ss;
        ss << "UsbDisconnectedEvent:  {guid: " << guid << ", name: " << name << "}";
        return wstring_to_utf8(ss.str());
    }

    EVENT_CLASS_TYPE(EventType::UsbDisconnected)
};
}    // namespace Brigerad
#endif    // BRIGERAD_SRC_BRIGERAD_EVENTS_USB_EVENT_H
