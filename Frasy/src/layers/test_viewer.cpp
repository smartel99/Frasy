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

void TestViewer::OnImGuiRender()
{
    if (!m_isVisible) { return; }
    if (ImGui::Begin("Test Viewer", &m_isVisible, ImGuiWindowFlags_NoDocking))
    {
        bool first = true;
        for (auto& sequence : m_sequences)
        {
            if (first) { first = false; }
            else { ImGui::Separator(); }
            RenderSequence(sequence);
        }

        if (m_sequences.empty())
        {
            ImGui::Text("Not loaded, is the test sequence generated?");
            if (ImGui::Button("Generate")) { m_interface->Generate(); }
        }
    }
    ImGui::End();
}

void TestViewer::RenderSequence(Models::Sequence& sequence)
{
    ImGui::PushID(sequence.name.c_str());
    ImGui::BeginTable(sequence.name.c_str(), 3, ImGuiTableFlags_SizingFixedFit);
    ImGui::TableSetupColumn("##Show", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
    ImGui::TableSetupColumn("##Name", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##Enabled", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);


    bool        sEnabled   = sequence.enabled;
    ListStatus& listStatus = m_listStatus[sequence.name];
    if (listStatus == ListStatus::unknown) { listStatus = sEnabled ? ListStatus::expanded : ListStatus::folded; }

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    if (ImGui::SmallButton(listStatus == ListStatus::expanded ? "v" : ">"))
    {
        listStatus = listStatus == ListStatus::expanded ? ListStatus::folded : ListStatus::expanded;
    }
    ImGui::TableNextColumn();
    ImGui::Text("%s", sequence.name.c_str());
    ImGui::TableNextColumn();
    if (ImGui::Checkbox("##enable", &sEnabled))
    {
        sequence.enabled = m_interface->SetSequenceEnable(sequence.name, sEnabled);
        listStatus       = sequence.enabled ? ListStatus::expanded : ListStatus::folded;
    }
    if (listStatus == ListStatus::expanded)
    {
        for (auto& test : sequence.tests)
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            ImGui::Text("%s", test.name.c_str());
            ImGui::TableNextColumn();
            bool tEnabled = test.enabled;
            if (ImGui::Checkbox("##", &tEnabled))
            {
                test.enabled = m_interface->SetTestEnable(sequence.name, test.name, tEnabled);
            }
        }
    }
    ImGui::EndTable();
    ImGui::PopID();
}

void TestViewer::OnStarted() {
    // TODO
}

void TestViewer::OnStopped() {
    // TODO
}

void TestViewer::OnGenerated(const std::vector<Models::Sequence>& sequences)
{
    m_sequences = sequences;
}
void TestViewer::OnTestPass(const std::string& sequence, const std::string& test)
{
    // TODO
}
void TestViewer::OnTestFail(const std::string& sequence, const std::string& test)
{
    // TODO
}
void TestViewer::OnTestSkipped(const std::string& sequence, const std::string& test)
{
    // TODO
}
void TestViewer::OnSequencePass(const std::string& sequence)
{
    // TODO
}
void TestViewer::OnSequenceFail(const std::string& sequence)
{
    // TODO
}
void TestViewer::OnSequenceSkipped(const std::string& sequence)
{
    // TODO
}

}    // namespace Frasy
