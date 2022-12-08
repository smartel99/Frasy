#pragma once

#include "Event.h"

#include "Brigerad/Scene/Components.h"

namespace Brigerad
{
class ImGuiButtonEvent : public Event
{
public:
    const Entity& GetButton() const { return m_button; }
    EVENT_CLASS_CATEGORY(EventCategoryImGui | EventCategoryInput)

protected:
    Entity m_button;
    ImGuiButtonEvent(const Entity& button) : m_button(button) {}
};

class BRIGERAD_API ImGuiButtonPressedEvent : public ImGuiButtonEvent
{
public:
    ImGuiButtonPressedEvent(const Entity& button) : ImGuiButtonEvent(button) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "ImGuiButtonPressedEvent: " << m_button.GetComponent<ImGuiButtonComponent>().name;
        return ss.str();
    }

    EVENT_CLASS_TYPE(EventType::ImGuiButtonPressed)
};

class BRIGERAD_API ImGuiButtonReleasedEvent : public ImGuiButtonEvent
{
public:
    ImGuiButtonReleasedEvent(const Entity& button) : ImGuiButtonEvent(button) {}

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "ImGuiButtonReleasedEvent: " << m_button.GetComponent<ImGuiButtonComponent>().name;
        return ss.str();
    }

    EVENT_CLASS_TYPE(EventType::ImGuiButtonReleased)
};
}    // namespace Brigerad
