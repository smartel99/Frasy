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

namespace Frasy::CanOpenViewer {

Layer::Layer(CanOpen::CanOpen& canOpen) noexcept : m_canOpen(canOpen)
{
}

void Layer::onAttach()
{
    Brigerad::Layer::onAttach();
}

void Layer::onDetach()
{
    Brigerad::Layer::onDetach();
}

void Layer::onUpdate(Brigerad::Timestep timestep)
{
    Brigerad::Layer::onUpdate(timestep);
}

void Layer::onImGuiRender()
{
    if (!m_isVisible) { return; }
    BR_PROFILE_FUNCTION();

    if (ImGui::Begin(s_windowName, &m_isVisible, ImGuiWindowFlags_NoDocking)) {
        ImGui::Text("%s", m_canOpen.isOpen()?"Active":"Innactive");
        ImGui::SameLine();
        if(ImGui::Button("Restart")) {
            m_canOpen.reset();
        }
        renderNodes();
    }
    ImGui::End();

    if (m_openNodes.empty()) { return; }

    bool open = true;
    if (ImGui::Begin("Node Explorer", &open)) {
        if (ImGui::BeginTabBar("nodeExplorerTabBar")) {
            for (auto&& node : m_openNodes) {
                node.onImGuiRender();
            }
            ImGui::EndTabBar();
        }

        std::erase_if(m_openNodes, [](const auto& node) { return !node.open(); });
    }
    ImGui::End();
    if (!open) { m_openNodes.clear(); }
}

void Layer::setVisibility(bool visibility)
{
    m_isVisible = visibility;
    ImGui::SetWindowFocus(s_windowName);
}

void Layer::renderNodes()
{
    BR_PROFILE_FUNCTION();

    Widget::Table("nodes", 3)
      .ColumnHeader("Name", ImGuiTableColumnFlags_DefaultSort)
      .ColumnHeader("Node ID")
      .ColumnHeader("State", ImGuiTableColumnFlags_WidthStretch)
      .ScrollFreeze()
      .FinishHeader()
      .Content(m_canOpen.getNodes(), [this](Widget::Table& table, const CanOpen::Node& node) {
          bool clicked    = false;
          auto renderCell = [&clicked](std::string_view str) {
              if (ImGui::Selectable(str.data())) { clicked = true; }
              if (ImGui::IsItemHovered()) {
                  ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
              }
          };

          table.CellContent(renderCell, node.name());
          table.CellContent(renderCell, std::format("0x{:02x}", node.nodeId()));
          table.CellContent(renderCell,
                            std::format("HB: {}, NMT: {}",
                                        CanOpen::toString(node.getHbState()),
                                        CanOpen::toString(node.getNmtState())));

          if (clicked && !std::ranges::any_of(
                           m_openNodes, [nodeId = node.nodeId()](const auto& n) { return n.nodeId() == nodeId; })) {
              m_openNodes.emplace_back(node.nodeId(), &m_canOpen);
          }
      });
}
}    // namespace Frasy::CanOpenViewer
