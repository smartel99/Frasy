/**
 * @file    can_open_viewer.cpp
 * @author  Samuel Martel
 * @date    2024-04-29
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
#include "can_open_viewer.h"

#include "utils/imgui/table.h"

namespace Frasy {

CanOpenViewer::CanOpenViewer(CanOpen::CanOpen& canOpen) noexcept : m_canOpen(canOpen)
{
}

void CanOpenViewer::onAttach()
{
    Layer::onAttach();
}

void CanOpenViewer::onDetach()
{
    Layer::onDetach();
}

void CanOpenViewer::onUpdate(Brigerad::Timestep timestep)
{
    Layer::onUpdate(timestep);
}

void CanOpenViewer::onImGuiRender()
{
    if (!m_isVisible) { return; }
    BR_PROFILE_FUNCTION();

    if (ImGui::Begin(s_windowName, &m_isVisible, ImGuiWindowFlags_NoDocking)) { renderNodes(); }
    ImGui::End();

    if (m_openNodes.empty()) { return; }

    bool open = true;
    if (ImGui::Begin("Node Explorer", &open)) {
        if (ImGui::BeginTabBar("nodeExplorerTabBar")) {
            for (auto&& node : m_openNodes) {
                // ImGui::BeginChildFrame(reinterpret_cast<ImGuiID>(&node), ImVec2 {0, 0});
                renderOpenNodeWindow(node);
                // ImGui::EndChildFrame();
            }
            ImGui::EndTabBar();
        }

        std::erase_if(m_openNodes, [](const auto& node) { return !node.open; });
    }
    ImGui::End();
    if (!open) { m_openNodes.clear(); }
}

void CanOpenViewer::setVisibility(bool visibility)
{
    m_isVisible = visibility;
    ImGui::SetWindowFocus(s_windowName);
}

void CanOpenViewer::renderNodes()
{
    BR_PROFILE_FUNCTION();

    Widget::Table("nodes", 3)
      .ColumnHeader("Name", ImGuiTableColumnFlags_DefaultSort)
      .ColumnHeader("Node ID")
      .ColumnHeader("State", ImGuiTableColumnFlags_WidthStretch)
      .ScrollFreeze(0, 1)
      .FinishHeader()    // Validate that if there was any calls to ColumnHeader, there was the right amount of them.
      .Content(
        m_canOpen.getNodes(), [this](Widget::Table& table, CanOpen::Node& node /* Just needs to be callable with T*/) {
            // Custom cell renderer.
            bool clicked    = false;
            auto renderCell = [&clicked](std::string_view str) {
                if (ImGui::Selectable(str.data())) { clicked = true; }
                if (ImGui::IsItemHovered()) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
                }
            };

            table.CellContent(renderCell, node.name());    // lambda needs to be callable with the arguments provided.
            table.CellContent(renderCell, std::format("0x{:02x}", node.nodeId()));
            table.CellContent(renderCell,
                              std::format("HB: {}, NMT: {}",
                                          CanOpen::toString(node.getHbState()),
                                          CanOpen::toString(node.getNmtState())));

            if (clicked && !std::ranges::any_of(
                             m_openNodes, [nodeId = node.nodeId()](const auto& n) { return n.nodeId == nodeId; })) {
                m_openNodes.emplace_back(true, std::format("{}tabBar", node.name()), node.nodeId());
            }
        });
}

bool CanOpenViewer::renderOpenNodeWindow(OpenNode& openNode)
{
    auto* node = m_canOpen.getNode(openNode.nodeId);
    if (node == nullptr) {
        openNode.open = false;
        return false;
    }

    if (ImGui::BeginTabItem(node->name().data(), &openNode.open)) {
        if (ImGui::BeginTabBar(openNode.tabBarName.c_str(),
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
            ImGui::EndTabBar();
        }
        ImGui::EndTabItem();
    }

    return openNode.open;
}

void CanOpenViewer::renderActiveErrors(const CanOpen::Node& node)
{
    BR_PROFILE_FUNCTION();
    Widget::Table(node.name().data(), 5)
      .ColumnHeader("Timestamp", ImGuiTableColumnFlags_DefaultSort)
      .ColumnHeader("Code")
      .ColumnHeader("Register")
      .ColumnHeader("Status")
      .ColumnHeader("Information")
      .FinishHeader()
      .Content(node.getEmergencies() | std::views::filter([](const auto& em) { return em.isActive; }),
               [](Widget::Table& table, const CanOpen::EmergencyMessage& em) {
                   if (em.isCritical()) { ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, s_criticalEmergencyColor); }
                   table.CellContenTextWrapped("{0:%c}", em.timestamp);
                   table.CellContenTextWrapped(em.errorCode);
                   table.CellContenTextWrapped(em.errorRegister);
                   table.CellContenTextWrapped(em.errorStatus);
                   table.CellContenTextWrapped(em.information);
               });
}

void CanOpenViewer::renderErrorHistory(const CanOpen::Node& node)
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
      .FinishHeader()
      .Content(node.getEmergencies(), [](Widget::Table& table, const CanOpen::EmergencyMessage& em) {
          if (em.isCritical()) { ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, s_criticalEmergencyColor); }
          table.CellContenTextWrapped("{0:%c %Z}", em.timestamp);
          table.CellContenTextWrapped(em.errorCode);
          table.CellContenTextWrapped(em.errorRegister);
          table.CellContenTextWrapped(em.errorStatus);
          table.CellContenTextWrapped(em.information);
          table.CellContenTextWrapped(em.isActive);
          table.CellContenTextWrapped("{0:%c %Z}", em.resolutionTime);
      });
}
}    // namespace Frasy
