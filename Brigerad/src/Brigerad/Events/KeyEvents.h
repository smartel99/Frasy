#pragma once

#include "Event.h"
#include "Brigerad/Core/KeyCodes.h"

namespace Brigerad
{
class BRIGERAD_API KeyEvent : public Event
{
public:
    inline KeyCode GetKeyCode() const
    {
        return m_keyCode;
    }

    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

protected:
    KeyEvent(KeyCode keycode) : m_keyCode(keycode)
    {
    }

    KeyCode m_keyCode;
};

class BRIGERAD_API KeyPressedEvent : public KeyEvent
{
public:
    KeyPressedEvent(KeyCode keycode, int repeatCount)
        : KeyEvent(keycode), m_repeatCount(repeatCount)
    {
    }

    inline int GetRepeatCount() const
    {
        return m_repeatCount;
    }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << m_keyCode << " (" << m_repeatCount << " repeats)";
        return ss.str();
    }

    EVENT_CLASS_TYPE(EventType::KeyPressed)

private:
    int m_repeatCount;
};

class BRIGERAD_API KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(KeyCode keycode)
        : KeyEvent(keycode)
    {
    }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyReleasedEvent: " << m_keyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(EventType::KeyReleased)
};

class BRIGERAD_API KeyTypedEvent : public KeyEvent
{
public:
    KeyTypedEvent(KeyCode keycode)
        : KeyEvent(keycode)
    {
    }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << m_keyCode;
        return ss.str();
    }

    EVENT_CLASS_TYPE(EventType::KeyTyped)

private:
};

} // namespace Brigerad
