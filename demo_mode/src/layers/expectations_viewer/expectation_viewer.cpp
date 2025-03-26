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
#include "utils/imgui/table.h"
#include "utils/lua/expectation.h"

void renderExpectations(const std::vector<Frasy::Lua::Expectation>& expectations)
{
    static constexpr auto tableFlags =
      ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
    static constexpr auto columnFlags = ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoHide |
                                        ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_NoReorder |
                                        ImGuiTableColumnFlags_NoSortAscending | ImGuiTableColumnFlags_NoSortDescending;

    Frasy::Widget::Table("ExpectationsTable", 5, tableFlags)
      .ColumnHeader("Name", columnFlags, 8.0f)
      .ColumnHeader("Measure", columnFlags, 3.0f)
      .ColumnHeader("Min", columnFlags, 3.0f)
      .ColumnHeader("Max", columnFlags, 3.0f)
      .ColumnHeader("Passed", columnFlags, 2.0f)
      .ScrollFreeze(0, 1)
      .FinishHeader()
      .Content(expectations, [](auto& table, const auto& e) {
          table.CellContentText(e.name);
          table.CellContentText(e.measure);
          table.CellContentText(e.min);
          table.CellContentText(e.max);
          table.CellContentText(e.pass ? "Yes" : "No");
      });
}
