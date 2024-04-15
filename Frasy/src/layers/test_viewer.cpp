/**
 * @file    test_viewer.cpp
 * @author  Paul Thomas
 * @date    5/2/2023
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

#include "test_viewer.h"

#include "imgui.h"

namespace Frasy
{

TestViewer::TestViewer() : m_interface(Interface::GetDefault())
{
}

void TestViewer::onImGuiRender()
{
    if (!m_isVisible) { return; }
    if (ImGui::Begin("Test Viewer", &m_isVisible, ImGuiWindowFlags_NoDocking))
    {
        bool                   first    = true;
        const Models::Solution& solution = m_interface->GetSolution();
        for (auto& [name, sequence] : solution.sequences)
        {
            if (first) { first = false; }
            else { ImGui::Separator(); }
            RenderSequence(name, sequence);
        }

        if (solution.sequences.empty())
        {
            ImGui::Text("Not loaded, is the test sequence generated?");
            if (ImGui::Button("Generate")) { m_interface->Generate(); }
        }
    }
    ImGui::End();
}

void TestViewer::RenderSequence(const std::string& sName, const Models::Sequence& sequence)
{
    ImGui::PushID(sName.c_str());
    ImGui::BeginTable(sName.c_str(), 3, ImGuiTableFlags_SizingFixedFit);
    ImGui::TableSetupColumn("##Show", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
    ImGui::TableSetupColumn("##Name", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##enabled", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);


    bool        sEnabled   = sequence.enabled;
    ListStatus& listStatus = m_listStatus[sName];
    if (listStatus == ListStatus::unknown) { listStatus = sEnabled ? ListStatus::expanded : ListStatus::folded; }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    if (ImGui::SmallButton(listStatus == ListStatus::expanded ? "v" : ">"))
    {
        listStatus = listStatus == ListStatus::expanded ? ListStatus::folded : ListStatus::expanded;
    }
    ImGui::TableNextColumn();
    ImGui::Text("%s", sName.c_str());
    ImGui::TableNextColumn();
    if (ImGui::Checkbox("##enable", &sEnabled))
    {
        m_interface->SetSequenceEnable(sName, sEnabled);
        listStatus = ListStatus::unknown;    // Will be recomputed next frame
    }
    if (listStatus == ListStatus::expanded)
    {
        for (auto& [tName, test] : sequence.tests)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            ImGui::Text("%s", tName.c_str());
            ImGui::TableNextColumn();
            bool tEnabled = test.enabled;
            if (ImGui::Checkbox("##", &tEnabled)) { m_interface->SetTestEnable(sName, tName, tEnabled); }
        }
    }
    ImGui::EndTable();
    ImGui::PopID();
}

TestViewer::Interface* TestViewer::Interface::GetDefault()
{
    static Interface interface;
    return &interface;
}

const Frasy::Models::Solution& TestViewer::Interface::GetSolution()
{
    static Frasy::Models::Solution solution;
    return solution;
}

void TestViewer::Interface::Generate()
{
    // Default empty call
}

void TestViewer::Interface::SetSequenceEnable(const std::string& sequence, bool enable)
{
    // Default empty call
}

void TestViewer::Interface::SetTestEnable(const std::string& sequence, const std::string& test, bool enable)
{
    // Default empty call
}

}    // namespace Frasy
