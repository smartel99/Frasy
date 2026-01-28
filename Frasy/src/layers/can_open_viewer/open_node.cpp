/**
 * @file    open_node.cpp
 * @author  Samuel Martel
 * @date    2024-05-08
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
#include "open_node.h"

#include "utils/imgui/table.h"

#include <Brigerad/Debug/Instrumentor.h>
#include <imgui.h>

namespace Frasy {

CanOpenViewer::OpenNode::OpenNode(uint8_t nodeId, CanOpen::CanOpen* canOpen)
: m_nodeId(nodeId), m_canOpen(canOpen), m_sdo(std::make_unique<Sdo>(nodeId))
{
    auto             maybeNode = canOpen->getNode(nodeId);
    std::string_view nodeName  = maybeNode.has_value() ? maybeNode.value()->name() : "Unknown";
    m_tabBarName               = std::format("{} - 0x{:2X}", nodeName, nodeId);
}

void CanOpenViewer::OpenNode::onImGuiRender()
{
    auto maybeNode = m_canOpen->getNode(m_nodeId);
    if (!maybeNode.has_value()) {
        m_open = false;
        return;
    }

    auto* node = maybeNode.value();

    if (ImGui::BeginTabItem(m_tabBarName.c_str(), &m_open)) {
        if (ImGui::BeginTabBar(m_tabBarName.c_str(),
                               ImGuiTabBarFlags_NoCloseWithMiddleMouseButton |
                                 ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_FittingPolicyScroll)) {
            if (ImGui::BeginTabItem("Active Errors")) {
                renderActiveErrors(*node);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Error History")) {
                renderErrorHistory(*node);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("SDO")) {
                m_sdo->onImGuiRender(*node);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::EndTabItem();
    }
}

void CanOpenViewer::OpenNode::renderActiveErrors(const CanOpen::Node& node)
{
    BR_PROFILE_FUNCTION();
    Widget::Table(node.name().data(), 5)
      .ColumnHeader("Timestamp", ImGuiTableColumnFlags_DefaultSort)
      .ColumnHeader("Code")
      .ColumnHeader("Register")
      .ColumnHeader("Status")
      .ColumnHeader("Information")
      .ScrollFreeze(0, 1)
      .FinishHeader()
      .Content(node.getEmergencies() | std::views::filter([](const auto& em) { return em.isActive; }),
               [](Widget::Table& table, const CanOpen::EmergencyMessage& em) {
                   if (em.isCritical()) { ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, s_criticalEmergencyColor); }
                   table.CellContentTextWrapped("{0:%c}", em.timestamp);
                   table.CellContentTextWrapped(em.errorCode);
                   table.CellContentTextWrapped(em.errorRegister);
                   table.CellContentTextWrapped(em.errorStatus);
                   table.CellContentTextWrapped(em.information);
               });
}

void CanOpenViewer::OpenNode::renderErrorHistory(const CanOpen::Node& node)
{
    BR_PROFILE_FUNCTION();
    Widget::Table(node.name().data(), 7)
      .ColumnHeader("Timestamp", ImGuiTableColumnFlags_DefaultSort)
      .ColumnHeader("Code")
      .ColumnHeader("Register")
      .ColumnHeader("Status")
      .ColumnHeader("Information")
      .ColumnHeader("Active")
      .ColumnHeader("Resolution Time")
      .ScrollFreeze(0, 1)
      .FinishHeader()
      .Content(node.getEmergencies(), [](Widget::Table& table, const CanOpen::EmergencyMessage& em) {
          if (em.isCritical()) { ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, s_criticalEmergencyColor); }
          table.CellContentTextWrapped("{0:%c %Z}", em.timestamp);
          table.CellContentTextWrapped(em.errorCode);
          table.CellContentTextWrapped(em.errorRegister);
          table.CellContentTextWrapped(em.errorStatus);
          table.CellContentTextWrapped(em.information);
          table.CellContentTextWrapped(em.isActive);
          table.CellContentTextWrapped("{0:%c %Z}", em.resolutionTime);
      });
}
}    // namespace Frasy
