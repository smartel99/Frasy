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

void CanOpenViewer::onImGuiRender()
{
    if (!m_isVisible) { return; }

    if (ImGui::Begin(s_windowName, &m_isVisible, ImGuiWindowFlags_NoDocking)) {
        static constexpr ImVec4 s_red   = {1, 0, 0, 1};
        static constexpr ImVec4 s_green = {0, 1, 0, 1};

        if (m_canOpen.m_redLed) { ImGui::PushStyleColor(ImGuiCol_Text, s_red); }
        else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        }
        ImGui::Text("Red");
        ImGui::PopStyleColor();

        ImGui::SameLine();

        if (m_canOpen.m_greenLed) { ImGui::PushStyleColor(ImGuiCol_Text, s_green); }
        else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        }
        ImGui::Text("Green");
        ImGui::PopStyleColor();

        if(ImGui::Button("Scan"))
        {
            m_canOpen.scanForDevices();
        }
    }
    ImGui::End();
}

void CanOpenViewer::setVisibility(bool visibility)
{
    m_isVisible = visibility;
    ImGui::SetWindowFocus(s_windowName);
}
}    // namespace Frasy
