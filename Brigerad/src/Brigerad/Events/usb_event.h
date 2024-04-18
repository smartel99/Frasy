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

#include "Event.h"

namespace Brigerad
{
class UsbEvent : public Event
{
public:
    EVENT_CLASS_CATEGORY(EventCategoryUsb)

protected:
    UsbEvent()  {}
};

class BRIGERAD_API UsbConnectedEvent : public UsbEvent
{
public:
    UsbConnectedEvent() : UsbEvent() {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "UsbConnectedEvent: ";
        return ss.str();
    }

    EVENT_CLASS_TYPE(EventType::UsbConnected)
};

class BRIGERAD_API UsbDisconnectedEvent : public UsbEvent
{
public:
    UsbDisconnectedEvent() : UsbEvent() {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "UsbDisconnectedEvent: ";
        return ss.str();
    }

    EVENT_CLASS_TYPE(EventType::UsbDisconnected)
};
}    // namespace Brigerad
#endif    // BRIGERAD_SRC_BRIGERAD_EVENTS_USB_EVENT_H
