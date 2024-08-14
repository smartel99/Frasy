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
#include <Brigerad/Core/Core.h>

#include <imgui.h>

#include <boost/pfr.hpp>

#include <concepts>
#include <cstddef>
#include <string_view>


namespace Frasy::Widget {
/**
 * Simple wrapper over ImGui's Table API.
 *
 * Example Usage:
 * @code
 * Widget::Table(node.name().data(), 7)
 * .ColumnHeader("Timestamp", ImGuiTableColumnFlags_DefaultSort)
 * .ColumnHeader("Code")
 * .ColumnHeader("Register")
 * .ColumnHeader("Status")
 * .ColumnHeader("Information")
 * .ColumnHeader("Active")
 * .ColumnHeader("Resolution Time")
 * .ScrollFreeze(0, 1)
 * .FinishHeader()
 * .Content(node.getEmergencies(), [](Widget::Table& table, const CanOpen::EmergencyMessage& em) {
 *     if (em.isCritical()) { ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, s_criticalEmergencyColor); }
 *     table.CellContenTextWrapped("{0:%c %Z}", em.timestamp);
 *     table.CellContenTextWrapped(em.errorCode);
 *     table.CellContenTextWrapped(em.errorRegister);
 *     table.CellContenTextWrapped(em.errorStatus);
 *     table.CellContenTextWrapped(em.information);
 *     table.CellContenTextWrapped(em.isActive);
 *     table.CellContenTextWrapped("{0:%c %Z}", em.resolutionTime);
 * });
 * @endcode
 */
class Table {
public:
    Table(std::string_view name, size_t columnCount, ImGuiTableFlags flags = s_defaultFlags)
    : Table(name, columnCount, ImVec2 {0.0f, ImGui::GetContentRegionAvail().y}, flags)
    {
    }
    Table(std::string_view name, size_t columnCount, ImVec2 size, ImGuiTableFlags flags = s_defaultFlags)
    : m_columnCount(columnCount), m_active(ImGui::BeginTable(name.data(), columnCount, flags, size))
    {
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
        if (m_active) {
            ImGui::TableSetupColumn(name.data(), flags, initWidthOrHeight, userId);
            ++m_realColumnCount;
        }
        return *this;
    }

    /**
     *
     * @param cols Column to be frozen
     * @param rows Row to be frozen
     * @return instance
     * @note By default (no arguments provided), freezes the first row.
     */
    Table& ScrollFreeze(int cols = 0, int rows = 1)
    {
        if (m_active) { ImGui::TableSetupScrollFreeze(cols, rows); }
        return *this;
    }

    /**
     * Renders the content of the table.
     *
     * @note View https://en.cppreference.com/w/cpp/ranges/viewable_range for a list of the acceptable types of
     * Containers.
     *
     * @tparam Container A range that can be converted into a view through views::all
     * @tparam Func A function to be called for each elements in the container. Needs to be invocable with an instance
     * of Table and an element of Container.
     * @param container A list of elements to be rendered in the table, where one element is one row
     * @param func A function that renders an element
     * @return instance
     */
    template<std::ranges::viewable_range Container, typename Func>
        requires requires(Container container) {
            requires std::invocable<Func,
                                    std::add_lvalue_reference_t<Table>,
                                    std::add_lvalue_reference_t<decltype(*std::begin(container))>>;
        }
    Table& Content(Container&& container, Func&& func)
    {
        if (!m_active) { return *this; }
        for (auto&& elem : container) {
            ImGui::TableNextRow();
            ImGui::BeginGroup();
            ImGui::PushID(&elem);
            std::invoke(std::forward<Func>(func), *this, std::forward<decltype(elem)>(elem));
            ImGui::PopID();
            ImGui::EndGroup();
        }
        return *this;
    }

    /**
     *
     * @tparam Container A range that can be converted into a view through views::all
     * @tparam ElemFunc A function to be called for each elements in the container. Needs to be invocable with an
     * instance of Table and an element of Container.
     * @tparam CellFunc A function to be called for each member of an element. Needs to be invocable with any member
     * type of the element.
     * @param container A list of elements to be rendered in the table, where one element is one row
     * @param elemFunc A function that is called before an element is rendered.
     * @param cellFunc A function that renders a member of an element.
     * @return instance
     *
     * @example
     * @code
     * struct S {
     *       std::string name;
     *       int id;
     *       float value;
     * };
     *
     * std::vector s = {
     *     S{"A", 1, 1.234f}, S{"B", 2, 5.678f}, S{"C", 3, 8.012f},
     *     S{"D", 4, 3.456f}, S{"E", 5, 7.890f},
     * };
     *
     * Table("My S Table", 3)
     *     .ColumnHeader("Name")
     *     .ColumnHeader("ID")
     *     .ColumnHeader("Value")
     *     .FinishHeader()
     *     .AutoContent(s,
     *          [](Table& table, const S& elem) {
     *              if(elem.id % 2) {
     *                  ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, 0x203030DC);
     *              }
     *          },
     *          []<typename T>(T&& t) {
     *              table.CellContentTextWrapped(std::forward<T>(t));
     *          });
     * @endcode
     */
    template<std::ranges::viewable_range Container, typename ElemFunc, typename CellFunc>
    Table& AutoContent(Container&& container, ElemFunc&& elemFunc, CellFunc&& cellFunc)
    {
        return Content(std::forward<Container>(container), [&cellFunc, &elemFunc](Table& table, auto&& elem) {
            std::invoke(elemFunc, table, elem);
            boost::pfr::for_each_field(
              elem, [&cellFunc]<typename T>(T&& item) { std::invoke(cellFunc, std::forward<T>(item)); });
        });
    }

    template<std::ranges::viewable_range Container, typename CellFunc>
    Table& AutoContent(Container&& container, CellFunc&& cellFunc)
    {
        return AutoContent(
          std::forward<Container>(container), [](Table&, const auto&) {}, std::forward<CellFunc>(cellFunc));
    }

    template<std::ranges::viewable_range Container, typename ElemFunc>
    Table& AutoContentElemFunc(Container&& container, ElemFunc&& elemFunc)
    {
        return AutoContent(std::forward<Container>(container), std::forward<ElemFunc>(elemFunc), []<typename T>(T&& t) {
            CellContentTextWrapped(std::forward<T>(t));
        });
    }

    template<std::ranges::viewable_range Container>
    Table& AutoContent(Container&& container)
    {
        return AutoContent(
          std::forward<Container>(container),
          [](Table&, const auto&) {},
          []<typename T>(T&& t) { CellContentTextWrapped(std::forward<T>(t)); });
    }

    template<typename Func, typename... Args>
        requires std::invocable<Func, Args...>
    static decltype(auto) CellContent(Func&& func, Args&&... args)
    {
        ImGui::TableNextColumn();
        return std::invoke(func, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    static void CellContentText(std::format_string<T, Args...> fmt, T&& t, Args&&... args)
    {
        CellContent(
          [](const std::format_string<T, Args...>& _fmt, T&& _t, Args&&... _args) {
              ImGui::Text("%s", std::format(_fmt, std::forward<T>(_t), std::forward<Args>(_args)...).c_str());
          },
          fmt,
          std::forward<T>(t),
          std::forward<Args>(args)...);
    }

    template<typename T>
    static void CellContentText(T&& t)
    {
        CellContentText("{}", std::forward<T>(t));
    }

    template<typename T, typename... Args>
    static void CellContentTextTooltip(std::format_string<T, Args...> fmt, T&& t, Args&&... args)
    {
        CellContent(
          [](const std::format_string<T, Args...>& _fmt, T&& _t, Args&&... _args) {
              std::string line = std::format(_fmt, std::forward<T>(_t), std::forward<Args>(_args)...);
              ImGui::Text("%s", line.c_str());
              if (ImGui::IsItemHovered()) {
                  ImGui::BeginTooltip();
                  ImGui::PushTextWrapPos(800.0f);
                  ImGui::Text(line.c_str());
                  ImGui::PopTextWrapPos();
                  ImGui::EndTooltip();
              }
          },
          fmt,
          std::forward<T>(t),
          std::forward<Args>(args)...);
    }

    template<typename T>
    static void CellContentTextTooltip(T&& t)
    {
        CellContentTextTooltip("{}", std::forward<T>(t));
    }

    template<typename T, typename... Args>
    static void CellContentTextWrapped(std::format_string<T, Args...> fmt, T&& t, Args&&... args)
    {
        CellContent(
          [](const std::format_string<T, Args...>& _fmt, T&& _t, Args&&... _args) {
              float wrapPos = ImGui::GetContentRegionMax().x;
              ImGui::PushTextWrapPos(wrapPos);
              ImGui::TextWrapped("%s", std::format(_fmt, std::forward<T>(_t), std::forward<Args>(_args)...).c_str());
              ImGui::PopTextWrapPos();
          },
          fmt,
          std::forward<T>(t),
          std::forward<Args>(args)...);
    }

    template<typename T>
    static void CellContentTextWrapped(T&& t)
    {
        CellContentTextWrapped("{}", std::forward<T>(t));
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
