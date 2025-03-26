/**
 * @file    expectation_viewer.cpp
 * @author  Paul Thomas
 * @date    3/25/2025
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

#include "expectations_viewer.h"
#include "utils/lua/expectation.h"
#include <imgui.h>

void renderExpectations(const std::vector<Frasy::Lua::Expectation>& expectations)
{
    static constexpr auto tableFlags =
      ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
    ImGui::BeginTable("ExpectationsTable", 5, tableFlags);
    static constexpr auto columnFlags = ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoHide |
                                        ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_NoReorder |
                                        ImGuiTableColumnFlags_NoSortAscending | ImGuiTableColumnFlags_NoSortDescending;
    ImGui::TableSetupColumn("Name", columnFlags, 8.0f);
    ImGui::TableSetupColumn("Measure", columnFlags, 3.0f);
    ImGui::TableSetupColumn("Min", columnFlags, 3.0f);
    ImGui::TableSetupColumn("Max", columnFlags, 3.0f);
    ImGui::TableSetupColumn("Passed", columnFlags, 2.0f);
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();
    for (const auto& expectation : expectations) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%s", expectation.name.c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%s", expectation.measure.c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%s", expectation.min.c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%s", expectation.max.c_str());
        ImGui::TableNextColumn();
        ImGui::Text("%s", expectation.pass ? "Yes" : "No");
    }
    ImGui::EndTable();
}
