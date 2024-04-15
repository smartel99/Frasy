/**
 * @file    result_analyzer.cpp
 * @author  Samuel Martel
 * @date    2023-05-02
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
#include "result_analyzer.h"

#include "imgui.h"
#include "utils/result_analyzer/analyzer.h"
#include "utils/result_analyzer/result_loader_saver.h"

#include <algorithm>
#include <Brigerad/Utils/dialogs/file.h>
#include <filesystem>

namespace Frasy
{
ResultAnalyzer::ResultAnalyzer() noexcept
{
}

void ResultAnalyzer::onImGuiRender()
{
    if (!m_isVisible) { return; }

    if (ImGui::Begin(s_windowName, &m_isVisible, ImGuiWindowFlags_NoDocking))
    {
        RenderStringList("Serial Numbers",
                         "List of the serial numbers to analyze. When empty, analyze all reports found.",
                         m_options.SerialNumbers);
        RenderStringList(
          "Locations", "List of the locations to analyze. When empty, analyze all locations found.", m_options.Uuts);
        RenderStringList("Sequences",
                         "List of the sequences to analyze. When empty, analyze all sequences found.",
                         m_options.Sequences);
        RenderStringList(
          "Tests", "List of the tests to analyze. When empty, analyze all tests found.", m_options.Tests);

        ImGui::Checkbox("Combine all locations", &m_options.Ganged);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
              "Combine all locations into the statistic report. When false, the statistics will be on a per-location "
              "basis.");
        }

        if (!m_generating)
        {
            if (ImGui::Button("Generate"))
            {
                m_analyzer        = Analyzers::ResultAnalyzer {m_options};
                m_doneGenerating  = false;
                m_generatorThread = std::thread(
                  [this]()
                  {
                      m_generating     = true;
                      m_lastResults    = m_analyzer.Analyze();
                      m_doneGenerating = true;
                      m_hasGenerated   = true;
                  });
            }
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Analyze all the reports with the given options."); }
            ImGui::SameLine();
            if (ImGui::Button("Show Last Report")) { m_renderResults = true; }
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Display the last generated analysis report."); }

            if (ImGui::Button("load Reports"))
            {
                BR_PROFILE_SCOPE("Loading Analysis Reports");
                auto pathsOpt =
                  Brigerad::Dialogs::OpenFiles("load Reports", {}, {"*.json"}, "Log Analysis Result Files");
                if (pathsOpt)
                {
                    for (auto&& path : *pathsOpt)
                    {
                        try
                        {
                            m_loadedResults[path] = Frasy::Analyzers::Load(path);
                        }
                        catch (std::exception& e)
                        {
                            BR_LOG_ERROR(s_windowName, "Unable to load '{}': {}", path, e.what());
                        }
                    }
                    m_renderResults = true;
                }
            }

            ImGui::SameLine();

            if (m_hasGenerated && ImGui::Button("Save Report"))
            {
                BR_PROFILE_SCOPE("Saving Analysis Report");
                auto suggestedPath = std::filesystem::current_path();
                suggestedPath /= "report.json";
                auto pathOpt =
                  Brigerad::Dialogs::SaveFile("Save File", suggestedPath.string(), {"*.json"}, "Log Analysis Results");
                if (pathOpt) { Frasy::Analyzers::Save(m_lastResults, *pathOpt); }
            }
        }
        else { ImGui::Text("Generating... %zu/%zu", m_analyzer.Analyzed, m_analyzer.ToAnalyze); }
        if (m_doneGenerating && m_generating)
        {
            m_generatorThread.join();
            m_generating    = false;
            m_renderResults = true;
        }

        if (m_renderResults) { RenderAnalysisResults(); }
    }
    ImGui::End();
}

void ResultAnalyzer::SetVisibility(bool visibility)
{
    m_isVisible = visibility;
}

void ResultAnalyzer::RenderStringList(std::string_view                   name,
                                      std::string_view                   tooltip,
                                      std::vector<std::array<char, 32>>& strings)
{
    if (ImGui::TreeNode(name.data()))
    {
        size_t at = 1;
        for (auto&& str : strings)
        {
            ImGui::InputTextWithHint(
              std::format("{}", at).c_str(), "Partial matches are supported.", str.data(), str.size());
            at++;
        }

        if (ImGui::Button("Add"))
        {
            if (strings.empty() || !strings.back().empty())
            {
                // Don't add a new string if there's already one with nothing in it.
                strings.push_back({});
            }
        }
        // TODO add support for removing strings.
        ImGui::TreePop();
    }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tooltip.data()); }
}

void ResultAnalyzer::RenderAnalysisResults()
{
    if (ImGui::Begin("Results", &m_renderResults))
    {
        if (m_loadedResults.empty()) { RenderSingleAnalysisResults(); }
        else { RenderMultipleAnalysisResults(); }
    }
    ImGui::End();
}

void ResultAnalyzer::RenderSingleAnalysisResults()
{
    if (ImGui::BeginTabBar("ResultLocationTabs"))
    {
        RenderAnalysisResultsFile(m_lastResults);
        ImGui::EndTabBar();
    }
}

void ResultAnalyzer::RenderMultipleAnalysisResults()
{
    if (ImGui::BeginTabBar("ResultFileTabs"))
    {
        if (ImGui::BeginTabItem("Last"))
        {
            ImGui::BeginTabBar("ResultLocationTabs");
            RenderAnalysisResultsFile(m_lastResults);
            ImGui::EndTabBar();
            ImGui::EndTabItem();
        }
        for (auto&& [name, result] : m_loadedResults)
        {
            if (ImGui::BeginTabItem(name.c_str()))
            {
                ImGui::BeginTabBar("ResultLocationTabs");
                RenderAnalysisResultsFile(result);
                ImGui::EndTabBar();
                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }
}

void ResultAnalyzer::RenderAnalysisResultsFile(const Analyzers::ResultAnalysisResults& results)
{
    for (auto&& [locationName, locationResults] : results.Locations)
    {
        if (ImGui::BeginTabItem(locationName.c_str()))
        {
            ImGui::BeginChild(locationName.c_str(), ImVec2 {0.0f, 0.0f}, false);
            RenderLocationAnalysisResults(locationResults);

            ImGui::EndChild();
            ImGui::EndTabItem();
        }
    }
}

void ResultAnalyzer::RenderLocationAnalysisResults(const Analyzers::ResultAnalysisResults::Location& location)
{
    ImGui::BulletText("Version: %s", location.Version.c_str());
    ImGui::BulletText(
      "Logs Analyzed: %zu, Passed:%zu (%0.2f%%)", location.Total, location.Passed, location.PassedPercent);

    const auto [min, max] = std::minmax_element(location.Durations.begin(), location.Durations.end());
    ImGui::BulletText(
      "Average Duration: %0.3f seconds (min: %0.3f seconds, max: %0.3f seconds)", location.AverageDuration, *min, *max);

    ImGui::Separator();

    ImGui::BeginTabBar(std::format("{}##sequences", location.Name).c_str());

    for (auto&& [name, sequence] : location.Sequences)
    {
        if (ImGui::BeginTabItem(name.c_str()))
        {
            ImGui::BeginChild(name.c_str(), ImVec2 {0.0f, 0.0f}, false);
            RenderSequenceAnalysisResults(sequence);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
    }
    ImGui::EndTabBar();
}

void ResultAnalyzer::RenderSequenceAnalysisResults(const Analyzers::ResultAnalysisResults::Sequence& sequence)
{
    ImGui::BulletText("Analyzed %zu times", sequence.Total);
    ImGui::BulletText("enabled %zu times (%0.2f%%), Passed %zu times (%0.2f%%), Skipped %zu times (%0.2f%%)",
                      sequence.Enabled,
                      sequence.EnabledPercent,
                      sequence.Passed,
                      sequence.PassedPercent,
                      sequence.Skipped,
                      sequence.SkippedPercent);

    const auto [min, max] = std::minmax_element(sequence.Durations.begin(), sequence.Durations.end());
    ImGui::BulletText(
      "Average Duration: %0.3f seconds (min: %0.3f seconds, max: %0.3f seconds)", sequence.AverageDuration, *min, *max);

    ImGui::Separator();

    ImGui::BeginTabBar(std::format("{}##tests", sequence.Name).c_str());

    for (auto&& [name, test] : sequence.Tests)
    {
        if (ImGui::BeginTabItem(name.c_str()))
        {
            ImGui::BeginChild(name.c_str(), ImVec2 {0.0f, 0.0f}, false);
            RenderTestAnalysisResults(test);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
    }

    ImGui::EndTabBar();
}

void ResultAnalyzer::RenderTestAnalysisResults(const Analyzers::ResultAnalysisResults::Test& test)
{
    ImGui::BulletText("Analyzed %zu times", test.Total);
    ImGui::BulletText("enabled %zu times (%0.2f%%), Passed %zu times (%0.2f%%), Skipped %zu times (%0.2f%%)",
                      test.Enabled,
                      test.EnabledPercent,
                      test.Passed,
                      test.PassedPercent,
                      test.Skipped,
                      test.SkippedPercent);


    const auto [min, max] = std::minmax_element(test.Durations.begin(), test.Durations.end());
    ImGui::BulletText(
      "Average Duration: %0.3f seconds (min: %0.3f seconds, max: %0.3f seconds)", test.AverageDuration, *min, *max);

    ImGui::Separator();

    ImGui::BeginTabBar(std::format("{}##tests", test.Name).c_str());

    for (auto&& [name, expectation] : test.Expectations)
    {
        if (ImGui::BeginTabItem(name.c_str()))
        {
            ImGui::BeginChild(name.c_str(), ImVec2 {0.0f, 0.0f}, false);
            expectation->Render();
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
    }

    ImGui::EndTabBar();
}

}    // namespace Frasy
