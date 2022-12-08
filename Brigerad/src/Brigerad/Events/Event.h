#pragma once
#include "Brigerad/Core/Core.h"

#include <functional>
#include <iostream>
#include <string>

namespace Brigerad
{
// Events in Brigerad are currently blocking, meaning when an event occurs,
// it immediately gets dispatched and must be dealt with right then and there.
// For the future, a better strategy might be to buffer events in an event
// bus and process them during the "event" part of the update stage.

/**
 * @enum    EventType
 * @brief   Lists all possible types of Event for Brigerad.
 */
enum class EventType
{
    None = 0,
    // Window Events. (The window on the screen, not OS events)
    WindowClose,
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    WindowMoved,
    // Application Events.
    AppTick,
    AppUpdate,
    AppRender,
    // Keyboard Events.
    KeyPressed,
    KeyReleased,
    KeyTyped,
    // Mouse Events.
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled,
    // ImGui Events.
    ImGuiButtonPressed,
    ImGuiButtonReleased,
};

enum EventCategory
{
    None                     = 0,
    EventCategoryApplication = BIT(0),
    EventCategoryInput       = BIT(1),
    EventCategoryKeyboard    = BIT(2),
    EventCategoryMouse       = BIT(3),
    EventCategoryMouseButton = BIT(4),
    EventCategoryImGui       = BIT(5),
};

#define EVENT_CLASS_TYPE(type)                                                                     \
    static EventType GetStaticType()                                                               \
    {                                                                                              \
        return type;                                                                               \
    }                                                                                              \
    virtual EventType GetEventType() const override                                                \
    {                                                                                              \
        return GetStaticType();                                                                    \
    }                                                                                              \
    virtual const char* GetName() const override                                                   \
    {                                                                                              \
        return #type;                                                                              \
    }

#define EVENT_CLASS_CATEGORY(category)                                                             \
    virtual int GetCategoryFlags() const override                                                  \
    {                                                                                              \
        return category;                                                                           \
    }

class BRIGERAD_API Event
{
    friend class EventDispatcher;

public:
    virtual ~Event() = default;

    // Pure virtual methods, they must be implemented by inheriting classes.
    virtual EventType   GetEventType() const     = 0;
    virtual const char* GetName() const          = 0;
    virtual int         GetCategoryFlags() const = 0;

    // Default, overridable method.
    virtual std::string ToString() const { return GetName(); }

// enum type 'Brigerad::EventCategory' is unscoped...
#pragma warning(disable : 26812)
    inline bool IsInCategory(EventCategory category)
#pragma warning(default : 26812)
    {
        return GetCategoryFlags() & category;
    }

    inline bool Handled() { return m_handled; }

    bool m_handled = false;
};

class EventDispatcher
{
    template<typename T>
    using EventFn = std::function<bool(T&)>;

public:
    EventDispatcher(Event& event) : m_event(event) {}

    template<typename T>
    bool Dispatch(EventFn<T> func)
    {
        if (m_event.GetEventType() == T::GetStaticType())
        {
            m_event.m_handled = func(*(T*)&m_event);
            return true;
        }
        return false;
    }

private:
    Event& m_event;
};

inline std::ostream& operator<<(std::ostream& os, const Event& e)
{
    return os << e.ToString();
}
}    // namespace Brigerad
