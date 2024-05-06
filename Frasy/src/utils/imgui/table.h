/**
 * @file    table.h
 * @author  Samuel Martel
 * @date    2024-05-06
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
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#ifndef FRASY_SRC_UTILS_IMGUI_TABLE_H
#define FRASY_SRC_UTILS_IMGUI_TABLE_H
#include <imgui.h>

#include <concepts>
#include <cstddef>
#include <string_view>


namespace Frasy::Widget {
class Table {
public:
    Table(std::string_view name, size_t columnCount, ImGuiTableFlags flags = s_defaultFlags)
    : Table(name, columnCount, ImVec2 {0.0f, ImGui::GetContentRegionAvail().y}, flags)
    {
    }
    Table(std::string_view name, size_t columnCount, ImVec2 size, ImGuiTableFlags flags = s_defaultFlags)
    : m_columnCount(columnCount)
    {
        m_active = ImGui::BeginTable(name.data(), columnCount, flags, size);
    }
    ~Table()
    {
        if (m_active) { Finish(); }
    }

    void Finish()
    {
        if (m_active) {
            ImGui::EndTable();
            m_active = false;
        }
    }
    Table& FinishHeader()
    {
        if (m_realColumnCount > 0) {
            BR_ASSERT(m_columnCount == m_realColumnCount,
                      "Mismatch between expected columns ({}) and actual columns ({})",
                      m_columnCount,
                      m_realColumnCount);
            ImGui::TableHeadersRow();
        }

        return *this;
    }

    Table& ColumnHeader(std::string_view      name,
                        ImGuiTableColumnFlags flags             = ImGuiTableColumnFlags_None,
                        float                 initWidthOrHeight = 0.0f,
                        ImGuiID               userId            = 0)
    {
        ImGui::TableSetupColumn(name.data(), flags, initWidthOrHeight, userId);
        ++m_realColumnCount;
        return *this;
    }

    /**
     *
     * @param cols Column to be frozen
     * @param rows Row to be frozen
     * @return instance
     */
    Table& ScrollFreeze(int cols, int rows)
    {
        ImGui::TableSetupScrollFreeze(cols, rows);
        return *this;
    }

    template<typename Container, typename Func>
        requires requires(Container container) {
            typename std::remove_reference_t<Container>::value_type;
            std::begin(container);
            std::end(container);
        } && std::invocable<Func,
                            std::add_lvalue_reference_t<Table>,
                            std::add_lvalue_reference_t<typename std::remove_cvref_t<Container>::value_type>>
    Table& Content(Container&& container, const Func& func)
    {
        for (auto&& elem : container) {
            ImGui::TableNextRow();
            ImGui::BeginGroup();
            ImGui::PushID(&elem);
            std::invoke(func, *this, std::forward<decltype(elem)>(elem));
            ImGui::PopID();
            ImGui::EndGroup();
        }
        return *this;
    }

    template<typename Func, typename... Args>
        requires std::invocable<Func, Args...>
    void CellContent(Func&& func, Args&&... args)
    {
        ImGui::TableNextColumn();
        std::invoke(func, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void CellContentText(std::format_string<Args...> fmt, Args&&... args)
    {
        CellContent(
          [](const std::format_string<Args...>& _fmt, Args&&... _args) {
            ImGui::Text("%s", std::format(_fmt, std::forward<Args>(_args)...).c_str());
          },
          fmt,
          std::forward<Args>(args)...);
    }

private:
    static constexpr ImGuiTableFlags s_defaultFlags =
      ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY |
      ImGuiTableFlags_SortMulti | ImGuiTableFlags_SortTristate | ImGuiTableFlags_SizingStretchProp;

private:
    size_t m_columnCount     = 0;
    size_t m_realColumnCount = 0;
    bool   m_active          = false;
};
}    // namespace Frasy::Widget

#endif    // FRASY_SRC_UTILS_IMGUI_TABLE_H
