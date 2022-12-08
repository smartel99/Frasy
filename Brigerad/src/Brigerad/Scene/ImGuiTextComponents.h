#pragma once

#include "Brigerad/Scene/Entity.h"

#include "imgui.h"

#include <string>
#include <vector>

namespace Brigerad
{
struct ImGuiTextComponent
{
    std::string text = "Placeholder";

    ImGuiTextComponent()                          = default;
    ImGuiTextComponent(const ImGuiTextComponent&) = default;
    ImGuiTextComponent(const std::string& t) : text(t) {}
};
}    // namespace Brigerad