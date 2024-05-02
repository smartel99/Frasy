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

    if (ImGui::Begin(s_windowName, &m_isVisible, ImGuiWindowFlags_NoDocking)) {
        renderNodes();

        for (auto&& [open, nodeId] : m_openNodes) {
            auto* node = m_canOpen.getNode(nodeId);
            if (node == nullptr) { open = false; }
            else {
                open = renderOpenNodeWindow(*node);
            }
        }

        std::erase_if(m_openNodes, [](const auto& node) { return !node.open; });
    }
    ImGui::End();
}

void CanOpenViewer::setVisibility(bool visibility)
{
    m_isVisible = visibility;
    ImGui::SetWindowFocus(s_windowName);
}

void CanOpenViewer::renderNodes()
{
    BR_PROFILE_FUNCTION();

    static ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable |
                                        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SortMulti |
                                        ImGuiTableFlags_SortTristate | ImGuiTableFlags_SizingStretchProp;

    float maxY = ImGui::GetContentRegionAvail().y;
    if (ImGui::BeginTable("nodes", 3, tableFlags, ImVec2 {0.0f, maxY})) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("Node ID");
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        for (auto&& node : m_canOpen.getNodes()) {
            ImGui::TableNextRow();
            renderNode(node);
        }
        ImGui::EndTable();
    }
}

void CanOpenViewer::renderNode(CanOpen::Node& node)
{
    BR_PROFILE_FUNCTION();
    static auto renderColumn = [this](size_t columnId, const char* str) {
        if (ImGui::TableSetColumnIndex(static_cast<int>(columnId))) {
            bool clicked = ImGui::Selectable(str);
            if (ImGui::IsItemHovered()) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
            }
            return clicked;
        }

        return false;
    };

    ImGui::BeginGroup();
    ImGui::PushID(ImGui::GetID(node.name().data()));
    bool clicked = renderColumn(0, node.name().data());
    clicked |= renderColumn(1, std::format("0x{:02x}", node.nodeId()).c_str());
    clicked |= renderColumn(
      2,
      std::format("HB: {}, NMT: {}", CanOpen::toString(node.getHbState()), CanOpen::toString(node.getNmtState()))
        .c_str());
    ImGui::PopID();
    ImGui::EndGroup();

    if (clicked &&
        !std::ranges::any_of(m_openNodes, [nodeId = node.nodeId()](const auto& n) { return n.nodeId == nodeId; })) {
        m_openNodes.emplace_back(true, node.nodeId());
    }
}

bool CanOpenViewer::renderOpenNodeWindow(CanOpen::Node& node)
{
    bool open = true;

    if (ImGui::Begin(node.name().data(), &open)) {
        for(auto&& em: node.getEmergencies()) {
            auto str = std::format("{}", em);
            ImGui::Text("%s", str.c_str());
            ImGui::Separator();
        }
    }
    ImGui::End();

    return open;
}
}    // namespace Frasy
