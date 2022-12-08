#pragma once

#include "Brigerad/Scene/Entity.h"

#include "imgui.h"

#include <string>
#include <vector>

namespace Brigerad
{
enum class ImGuiButtonState
{
    Inactive = 0,
    Pressed  = 1,
    Held     = 2,
    Released = 3
};

template<typename T>
struct ImGuiButtonListenerComponent
{
private:
    Entity button;

public:
    ImGuiButtonListenerComponent()                                    = default;
    ImGuiButtonListenerComponent(const ImGuiButtonListenerComponent&) = default;
    ImGuiButtonListenerComponent(const Entity& b) : button(b) {}

    bool IsButton(const Entity& other) const { return button == other; }
    bool IsButtonPressed()
    {
        return button.GetComponentRef<T>().state == ImGuiButtonState::Pressed;
    }
    bool IsButtonHeld() { return button.GetComponentRef<T>().state == ImGuiButtonState::Held; }
    bool IsButtonReleased()
    {
        return button.GetComponentRef<T>().state == ImGuiButtonState::Released;
    }
};

struct ImGuiButtonComponent
{
    std::string      name  = "Button";
    ImGuiButtonState state = ImGuiButtonState::Inactive;

    ImGuiButtonComponent()                            = default;
    ImGuiButtonComponent(const ImGuiButtonComponent&) = default;
    ImGuiButtonComponent(const std::string& n) : name(n) {}

    void* GetImGuiID() { return reinterpret_cast<void*>(this); }
    void* GetImGuiID() const { return const_cast<void*>(reinterpret_cast<const void*>(this)); }
};

struct ImGuiSmallButtonComponent
{
    std::string      name  = "Button";
    ImGuiButtonState state = ImGuiButtonState::Inactive;

    ImGuiSmallButtonComponent()                                 = default;
    ImGuiSmallButtonComponent(const ImGuiSmallButtonComponent&) = default;
    ImGuiSmallButtonComponent(const std::string& n) : name(n) {}

    void* GetImGuiID() { return reinterpret_cast<void*>(this); }
    void* GetImGuiID() const { return const_cast<void*>(reinterpret_cast<const void*>(this)); }
};

struct ImGuiInvisibleButtonComponent
{
    std::string      name  = "Button";
    ImGuiButtonState state = ImGuiButtonState::Inactive;
    ImGuiButtonFlags flag  = ImGuiButtonFlags_MouseButtonLeft;
    ImVec2           size  = ImVec2(0.1f, 0.1f);    // ImVec2(0, 0) is invalid for InvisibleButton.

    ImGuiInvisibleButtonComponent()                                     = default;
    ImGuiInvisibleButtonComponent(const ImGuiInvisibleButtonComponent&) = default;
    ImGuiInvisibleButtonComponent(const std::string& n, const ImVec2& s = ImVec2(0, 0)) : name(n)
    {
        if (s.x == 0.0f && s.y == 0.0f)
        {
            size = ImGui::CalcTextSize(n.c_str());
        }
        else
        {
            size = s;
        }
    }

    void* GetImGuiID() { return reinterpret_cast<void*>(this); }
    void* GetImGuiID() const { return const_cast<void*>(reinterpret_cast<const void*>(this)); }
};

struct ImGuiArrowButtonComponent
{
    std::string      name      = "Button";
    ImGuiButtonState state     = ImGuiButtonState::Inactive;
    ImGuiDir         direction = ImGuiDir_Left;

    ImGuiArrowButtonComponent()                                 = default;
    ImGuiArrowButtonComponent(const ImGuiArrowButtonComponent&) = default;
    ImGuiArrowButtonComponent(const std::string& n, ImGuiDir dir = ImGuiDir_Left)
    : name(n), direction(dir)
    {
    }

    void* GetImGuiID() { return reinterpret_cast<void*>(this); }
    void* GetImGuiID() const { return const_cast<void*>(reinterpret_cast<const void*>(this)); }
};
}    // namespace Brigerad