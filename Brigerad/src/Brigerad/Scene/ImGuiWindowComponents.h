#pragma once

#include "Brigerad/Scene/Entity.h"

#include "imgui.h"

#include <string>
#include <vector>

namespace Brigerad
{
struct ImGuiWindowComponent
{

    std::string         name   = "Window";
    bool                isOpen = true;
    ImGuiWindowFlags    flags  = ImGuiWindowFlags_None;
    std::vector<Entity> childs;

    ImGuiWindowComponent()                            = default;
    ImGuiWindowComponent(const ImGuiWindowComponent&) = default;
    ImGuiWindowComponent(const std::string& n, ImGuiWindowFlags f = ImGuiWindowFlags_None)
    : name(n), flags(f)
    {
    }

    void AddChildEntity(Entity child) { childs.emplace_back(child); }
};
}    // namespace Brigerad
