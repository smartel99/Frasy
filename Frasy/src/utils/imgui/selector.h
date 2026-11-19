/**
 * @file    selector.h
 * @author  Paul Thomas
 * @date    2026-11-19
 * @brief   Dropdown with previous and next arrows, to pick one item of a list.
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this
 * program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef FRASY_UTILS_IMGUI_SELECTOR_H
#define FRASY_UTILS_IMGUI_SELECTOR_H

#include <imgui.h>

#include <cfloat>
#include <cstddef>
#include <string>

namespace Frasy::Widget {

/**
 * @brief Previous and next arrows with a dropdown, like the selectors of the Result Analyzer.
 * @param label    Identifies the selector. Prefix it with `##` to hide it.
 * @param selected Index of the selected item, updated on selection.
 * @param items    Items to choose from. Must not be empty, and `selected` must be a valid index.
 * @param nameOf   Gives the displayed name of an item, as a string (or a reference to one).
 * @return True if the selection changed.
 */
template<typename Items, typename NameOf>
bool Selector(const char* label, std::size_t& selected, const Items& items, NameOf nameOf)
{
    bool changed = false;

    ImGui::PushID(label);
    if (ImGui::ArrowButton("##previous", ImGuiDir_Left) && selected > 0) {
        --selected;
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::ArrowButton("##next", ImGuiDir_Right) && selected + 1 < items.size()) {
        ++selected;
        changed = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo(label, nameOf(items[selected]).c_str())) {
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (ImGui::Selectable(nameOf(items[i]).c_str(), selected == i)) {
                changed  = changed || selected != i;
                selected = i;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();
    return changed;
}

}    // namespace Frasy::Widget

#endif    // FRASY_UTILS_IMGUI_SELECTOR_H
