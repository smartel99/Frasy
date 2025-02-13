/**
 * @file    circular_progress_indicator.cpp
 * @author  Samuel Martel
 * @date    2024-10-23
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
#include "circular_progress_indicator.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <numbers>

namespace Frasy::Widget {

void circularProgressIndicator(const char* id, float fraction, float radius, float thickness, std::string_view label)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) { return; }

    float  thicc2 = thickness * 2.0f;
    ImVec2 tl =
      ImVec2(window->DC.CursorPos.x + thicc2, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset + thicc2);
    ImVec2 br     = ImVec2(tl.x + (radius * 2.0f), tl.y + (radius * 2.0f));
    ImRect bb     = ImRect(tl, br);
    ImVec2 center = bb.GetCenter();

    ImGui::ItemSize(bb.GetSize());
    if (!ImGui::ItemAdd(bb, ImGui::GetID(id))) { return; }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddCircle(center, radius, ImGui::GetColorU32(ImGuiCol_FrameBg), 0, thickness);
    static constexpr float top      = 3.0f * std::numbers::pi_v<float> / 2.0f;
    float                  progress = fraction * 2.0f * std::numbers::pi_v<float>;
    drawList->PathArcTo(center, radius, top, progress + top);
    drawList->PathStroke(ImGui::GetColorU32(ImGuiCol_FrameBgActive), 0, thickness);

    if (!label.empty()) {
        // CalcTextSize gives us the size of the box that contains the text that needs to be rendered.
        // We need to make a bounding box that fits this text, offset from the center of the circle.
        ImGuiIO& io = ImGui::GetIO();
        ImGui::PushFont(io.Fonts->Fonts[1]);    // Larger sized font
        ImVec2 textSize = ImGui::CalcTextSize(label.data(), label.data() + label.size(), false, bb.GetWidth());
        ImGui::RenderTextWrapped(ImVec2(center.x - (textSize.x / 2.0f), center.y - (textSize.y / 2.0f)),
                                 label.data(),
                                 label.data() + label.size(),
                                 bb.GetWidth());
        ImGui::PopFont();
    }
}

bool circularProgressIndicatorButton(
  const char* id, float fraction, float radius, float thickness, std::string_view label)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) { return false; }

    ImGui::PushID(id);
    float  thicc2 = thickness * 2.0f;
    ImVec2 tl =
      ImVec2(window->DC.CursorPos.x + thicc2, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset + thicc2);
    ImVec2  br   = ImVec2(tl.x + (radius * 2.0f), tl.y + (radius * 2.0f));
    ImRect  bb   = ImRect(tl, br);
    ImGuiID imId = ImGui::GetID(id);

    bool hovered = false;
    bool held    = false;
    bool pressed = ImGui::ButtonBehavior(bb, imId, &hovered, &held, ImGuiButtonFlags_PressedOnClickRelease);
    // ButtonBehavior doesn't seem to consistently behave like a button...
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { pressed = true; }

    if (hovered) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddCircleFilled(bb.GetCenter(),
                                  radius - (thickness / 2.0f),
                                  ImGui::GetColorU32((pressed || held) ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBg));
        ImGui::PushStyleColor(ImGuiCol_FrameBg,
                              ImGui::GetColorU32((pressed || held) ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBgHovered));
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    circularProgressIndicator("_innerCPI", fraction, radius, thickness, label);

    if (hovered) { ImGui::PopStyleColor(); }

    ImGui::PopID();
    return pressed;
}
}    // namespace Frasy::Widget
