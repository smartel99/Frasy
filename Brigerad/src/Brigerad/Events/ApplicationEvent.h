#pragma once

#include "Brigerad/Events/Event.h"

namespace Brigerad
{
class BRIGERAD_API WindowResizeEvent : public Event
{
public:
    WindowResizeEvent(unsigned int width, unsigned int height)
        : m_width(width), m_height(height)
    {
    }

    inline const unsigned int GetWidth() const
    {
        return m_width;
    }

    inline const unsigned int GetHeight() const
    {
        return m_height;
    }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "WindowResizeEvent: " << m_width << ", " << m_height;
        return ss.str();
    }

    EVENT_CLASS_TYPE(EventType::WindowResize);
    EVENT_CLASS_CATEGORY(EventCategoryApplication);

private:
    unsigned int m_width, m_height;
};

class BRIGERAD_API WindowCloseEvent : public Event
{
public:
    WindowCloseEvent() = default;

    EVENT_CLASS_TYPE(EventType::WindowClose);
    EVENT_CLASS_CATEGORY(EventCategoryApplication);
};

class BRIGERAD_API AppTickEvent : public Event
{
public:
    AppTickEvent() = default;

    EVENT_CLASS_TYPE(EventType::AppTick);
    EVENT_CLASS_CATEGORY(EventCategoryApplication);
};

class BRIGERAD_API AppUpdateEvent : public Event
{
public:
    AppUpdateEvent() = default;

    EVENT_CLASS_TYPE(EventType::AppUpdate);
    EVENT_CLASS_CATEGORY(EventCategoryApplication);
};

class BRIGERAD_API AppRenderEvent : public Event
{
public:
    AppRenderEvent() = default;

    EVENT_CLASS_TYPE(EventType::AppUpdate);
    EVENT_CLASS_CATEGORY(EventCategoryApplication);
};

} // namespace Brigerad
