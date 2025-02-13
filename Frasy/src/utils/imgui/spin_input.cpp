/**
 * @file    spin_input.cpp
 * @author  Samuel Martel
 * @date    2024-10-22
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
#include "spin_input.h"

#include <Brigerad/Core/Core.h>
#include <imgui_internal.h>

#include <format>
#include <string>

// The implementation of spinScalar has been adapted from https://github.com/ocornut/imgui/issues/2649

namespace Frasy::Widget {

namespace {
bool incrementButton(int* value, int min, int max, int step)
{
    bool buttonPressed = false;
    if (step == 1) { buttonPressed = ImGui::ArrowButton("+", ImGuiDir_Up); }
    else {
        std::string label = std::format("+{}", step);
        buttonPressed     = ImGui::Button(label.c_str());
    }

    if (buttonPressed) {
        if (*value + step > max) { *value = min + std::abs((*value + step) - max) - 1; }
        else {
            *value += step;
        }
        return true;
    }
    return false;
}

bool decrementButton(int* value, int min, int max, int step)
{
    bool buttonPressed = false;
    if (step == 1) { buttonPressed = ImGui::ArrowButton("-", ImGuiDir_Down); }
    else {
        std::string label = std::format("-{}", step);
        buttonPressed     = ImGui::Button(label.c_str());
    }

    if (buttonPressed) {
        if (*value - step < min) { *value = max - std::abs(min - (*value - step)) + 1; }
        else {
            *value -= step;
        }
        return true;
    }
    return false;
}
}    // namespace

bool spinInt(const char* label, int* v, int step, int stepFast, int min, int max, const char* format)
{
    BR_ASSERT(min < max, "Min must be less than max!");
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) { return false; }

    ImGuiContext& g = *GImGui;

    ImGui::BeginGroup();
    ImGui::PushID(label);
    ImGui::BeginVertical(label, ImVec2(0, ImGui::GetFrameHeightWithSpacing()), 0.5f);
    bool hasChanged = false;
    hasChanged |= incrementButton(v, min, max, g.IO.KeyCtrl ? stepFast : step);
    ImGui::Spring();
    ImGui::Text(format, *v);
    if (ImGui::IsItemHovered()) {
        // TODO change value by `step` on mouse wheel usage.
    }
    ImGui::Spring();
    hasChanged |= decrementButton(v, min, max, g.IO.KeyCtrl ? stepFast : step);
    ImGui::EndVertical();
    ImGui::PopID();
    ImGui::EndGroup();
    return hasChanged;
}

}    // namespace Frasy::Widget
