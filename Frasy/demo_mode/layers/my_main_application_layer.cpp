/**
 * @file    my_main_application_layer.cpp
 * @author  Samuel Martel
 * @date    2022-12-05
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
#include "my_main_application_layer.h"

#include <regex>

MyMainApplicationLayer::MyMainApplicationLayer() : MainApplicationLayer()
{
}

void MyMainApplicationLayer::OnAttach()
{
    MainApplicationLayer::OnAttach();
    m_orchestrator.Init("lua/user/environment", "lua/user/tests");
    m_orchestrator.EnableIb(false);
    m_map = m_orchestrator.GetMap();
}

void MyMainApplicationLayer::OnDetach()
{
    MainApplicationLayer::OnDetach();
}

void MyMainApplicationLayer::RenderControlRoom()
{
    ImGui::Begin("Control Room");
    {
        uint64_t texture;
        if (Frasy::Communication::DeviceMap::Get().IsScanning()) { texture = m_waiting->GetRenderID(); }
        else if (m_orchestrator.IsRunning()) { texture = m_testing->GetRenderID(); }
        else { texture = m_run->GetRenderID(); }
        ImGui::BeginTable("Launcher", 2);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        static const ImVec2 buttonSize = ImVec2 {100.0f, 100.0f};
        static const ImVec2 buttonUv0  = ImVec2 {0.0f, 1.0f};
        static const ImVec2 buttonUv1  = ImVec2 {1.0f, 0.0f};
        if (ImGui::ImageButton(reinterpret_cast<void*>(texture), buttonSize, buttonUv0, buttonUv1)) { DoTests(); }
        ImGui::TableNextColumn();
        ImGui::InputText("Operator", m_operator, operatorLength);
        if (ImGui::InputText("Serial Number - Top Left", m_serialNumberTopLeft, serialNumberLength))
        {
            m_serialIsDirty = false;
        }
        if (ImGui::InputText("Serial Number - Bottom Right", m_serialNumberBottomRight, serialNumberLength))
        {
            m_serialIsDirty = false;
        }
        if (ImGui::Button("Scan serials")) {}
        ImGui::EndTable();
    }

    ImGui::BeginTable("UUT", m_map.ibs.size());
    for (const auto& [index, ib] : m_map.ibs)
    {
        ImGui::TableNextRow();
        for (const auto& [leader, team] : ib.teams)
        {
            ImGui::TableNextColumn();
            ImGui::PushID(std::format("Team{}", leader).c_str());
            ImGui::BeginTable("Team", team.uuts.size());
            ImGui::TableNextRow();
            for (const auto& uut : team.uuts)
            {
                ImGui::TableNextColumn();
                ImGui::PushID(std::format("UUT{}", uut).c_str());
                static const ImVec2 buttonSize = ImVec2 {80.0f, 80.0f};
                static const ImVec2 buttonUv0  = ImVec2 {0.0f, 1.0f};
                static const ImVec2 buttonUv1  = ImVec2 {1.0f, 0.0f};
                ImGui::Text("UUT %zu", uut);
                auto     state = m_orchestrator.UutState(uut);
                uint64_t texture;
                switch (state)
                {
                    case Frasy::UutState::Disabled: texture = m_disabled->GetRenderID(); break;
                    case Frasy::UutState::Idle: texture = m_idle->GetRenderID(); break;
                    case Frasy::UutState::Waiting: texture = m_waiting->GetRenderID(); break;
                    case Frasy::UutState::Running: texture = m_testing->GetRenderID(); break;
                    case Frasy::UutState::Passed: texture = m_pass->GetRenderID(); break;
                    case Frasy::UutState::Failed: texture = m_fail->GetRenderID(); break;
                    case Frasy::UutState::Error: texture = m_error->GetRenderID(); break;
                }
                if (ImGui::ImageButton(
                      reinterpret_cast<void*>(static_cast<uint64_t>(texture)), buttonSize, buttonUv0, buttonUv1))
                {
                    m_orchestrator.ToggleUut(uut);
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
            ImGui::PopID();
        }
    }
    ImGui::EndTable();
    ImGui::End();

    m_orchestrator.RenderPopups();
}

void MyMainApplicationLayer::DoTests()
{
    if (!GetSerials()) { return; }
    m_orchestrator.DoTests(m_serials);
}

bool MyMainApplicationLayer::GetSerials()
{
    m_serials.clear();
    std::regex  snRe("(.+)([0-9A-F]{3})$");
    std::cmatch matches;
    std::string snPrefix;
    int         snStart;
    int         snEnd;
    if (std::regex_search(m_serialNumberTopLeft, matches, snRe))
    {
        snPrefix = matches[1];
        snStart  = std::stoi(matches[2], nullptr, 10);
    }
    else { return false; }
    if (std::regex_search(m_serialNumberBottomRight, matches, snRe))
    {
        if (snPrefix != matches[1]) { return false; }
        snEnd = std::stoi(matches[2], nullptr, 10);
    }
    else { return false; }
    auto map = m_orchestrator.GetMap();
    if (snEnd - snStart != map.count.uut - 1) { return false; }
    m_serials.reserve(map.count.uut + 1);
    m_serials.emplace_back();
    for (int i = snStart; i <= snStart + map.count.uut - 1; ++i)
    {
        m_serials.push_back(std::format("{}{:03x}", snPrefix, i));
    }
    return true;
}
