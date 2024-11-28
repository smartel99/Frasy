/**
 * @file    result_viewer.cpp
 * @author  Samuel Martel
 * @date    2023-05-01
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
#include "result_viewer.h"

#include "imgui.h"

namespace Frasy {

ResultViewer::ResultViewer() noexcept : m_logs(LoadLogs(true))
{
}

void ResultViewer::onImGuiRender()
{
    if (!m_isVisible) { return; }

    if (ImGui::Begin(s_windowName, &m_isVisible, ImGuiWindowFlags_NoDocking)) {
        if (AreLogsNew()) {
            // One or more files have been modified, reload their data.
            std::for_each(m_logs.begin(), m_logs.end(), [](auto& log) {
                try {
                    log.Results = LoadResults(log.Path);
                    log.IsGood  = true;
                }
                catch (std::exception& e) {
                    BR_LOG_ERROR(s_windowName, "Unable to load results from '{}': {}", log.Name, e.what());
                    log.IsGood = false;
                }
            });
            m_isFirstPassOfLogs = true;
        }

        if (ImGui::BeginTabBar("ResultTabBar",
                               ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll)) {
            for (auto&& log : m_logs) {
                bool passed = log.Results.Passed;
                if (!passed) { ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF); }
                if (ImGui::BeginTabItem(log.Name.c_str())) {
                    if (!passed) { ImGui::PopStyleColor(); }
                    if (ImGui::BeginChild(log.Name.c_str(), ImVec2 {0.0f, 0.0f}, false)) {
                        if (log.IsGood) { RenderLog(log.Results); }
                        else {
                            ImGui::Text("Log is not valid");
                        }
                    }
                    ImGui::EndChild();

                    ImGui::EndTabItem();
                }
                else {
                    if (!passed) { ImGui::PopStyleColor(); }
                }
            }

            ImGui::EndTabBar();
        }
        m_isFirstPassOfLogs = false;
    }
    ImGui::End();
}

void ResultViewer::RenderLog(const OverallTestResult& log)
{
    ImGui::Text("Serial Number: %s, UUT %d - %s", log.SerialNumber.c_str(), log.Uut, log.Passed ? "PASSED" : "FAILED");
    ImGui::Text("Date: %s, Duration: %0.3f seconds", log.Date.c_str(), log.Duration);
    ImGui::Text("Version: %s", log.Version.c_str());

    for (auto&& [sequenceName, sequence] : log.Sequences) {
        if (m_isFirstPassOfLogs) { ImGui::SetNextItemOpen(!sequence.Passed, ImGuiCond_Always); }
        if (ImGui::TreeNode(sequenceName.c_str())) {
            RenderSequence(sequence);
            ImGui::TreePop();
        }
    }
}

void ResultViewer::RenderSequence(const ResultViewer::SequenceResult& sequence)
{
    ImGui::Text("Name: %s - %s", sequence.Name.c_str(), sequence.Passed ? "PASSED" : "FAILED");
    ImGui::Text("Skipped: %s, enabled: %s", sequence.Skipped ? "True" : "False", sequence.Enabled ? "True" : "False");
    ImGui::Text("Duration: %0.3f seconds", sequence.Duration);

    for (auto&& [testName, test] : sequence.Tests) {
        if (m_isFirstPassOfLogs) { ImGui::SetNextItemOpen(!test.Passed, ImGuiCond_Always); }
        if (ImGui::TreeNode(testName.c_str())) {
            RenderTest(test);
            ImGui::TreePop();
        }
        ImGui::Separator();
    }
}

void ResultViewer::RenderTest(const ResultViewer::TestResult& test)
{
    ImGui::Text("Name: %s - %s", test.Name.c_str(), test.Passed ? "PASSED" : "FAILED");
    ImGui::Text("Skipped: %s, enabled: %s", test.Skipped ? "True" : "False", test.Enabled ? "True" : "False");
    ImGui::Text("Duration: %0.3f seconds", test.Duration);

    if (m_isFirstPassOfLogs) { ImGui::SetNextItemOpen(!test.Passed, ImGuiCond_Always); }
    if (ImGui::TreeNode("Expectations")) {
        size_t at = 1;
        for (auto&& expectation : test.Expectations) {
            if (m_isFirstPassOfLogs) {
                bool passed = true;
                if (expectation.contains("pass")) { passed = expectation.at("pass").get<bool>(); }
                ImGui::SetNextItemOpen(!passed, ImGuiCond_Always);
            }
            std::string label = std::format("Expectation {}", at);
            if (expectation.contains("note")) { label = expectation.at("note").get<std::string>(); }
            if (ImGui::TreeNode(&expectation, "%s", label.c_str())) {
                RenderExpectation(expectation);
                ImGui::TreePop();
            }
            at++;
        }
        ImGui::TreePop();
    }
}

void ResultViewer::RenderExpectation(const ResultViewer::ExpectationDetails& expectation)
{
    for (const auto& [key, value] : expectation) {
        ImGui::TextWrapped("%s", MakeStringFromJson(key, value).c_str());
    }
}

void ResultViewer::setVisibility(bool visibility)
{
    m_isVisible = visibility;
    ImGui::SetWindowFocus(s_windowName);
}

bool ResultViewer::AreLogsNew()
{
    std::vector<LogInfo> currentLastModifiedDate = LoadLogs(false);

    if (std::ranges::any_of(currentLastModifiedDate, [this](const LogInfo& entry) {
            // Find the log in the logs we currently know.
            auto it = std::find_if(m_logs.begin(), m_logs.end(), [&entry](const auto& currentEntry) {
                return entry.Name == currentEntry.Name;
            });
            // If the log isn't known, we need to refresh them!
            if (it == m_logs.end()) { return true; }
            return it->LastModified < entry.LastModified;
        })) {
        m_logs = currentLastModifiedDate;
        return true;
    }
    return false;
}

std::vector<ResultViewer::LogInfo> ResultViewer::LoadLogs(bool loadFiles)
{
    // For each file in the last log directory:
    // - Check if the entry is a file
    // - Check if the file is a json file
    // - Add it to the list if that's the case.
    namespace fs = std::filesystem;
    // First, check if the last log directory exists in the first place.
    if (!fs::exists(s_lastLogsPath)) {
        // Directory doesn't exist!
        return {};
    }

    std::vector<LogInfo> infos;

    // Check if each entry is a json file.
    for (const auto& entry : fs::recursive_directory_iterator(s_lastLogsPath)) {
        if (entry.is_regular_file() && entry.path().extension() == s_logFileExtension) {
            std::string path = entry.path().string();
            try {
                OverallTestResult results = {};
                if (loadFiles) { results = LoadResults(path); }
                infos.emplace_back(entry.path().stem().string(), path, entry.last_write_time(), results);
            }
            catch (std::exception& e) {
                BR_APP_ERROR("An error occurred while parsing log '{}': {}", path, e.what());
                return {};
            }
        }
    }

    return infos;
}

ResultViewer::OverallTestResult ResultViewer::LoadResults(const std::string& path)
{
    auto loadJson = [](const std::string& p) -> nlohmann::json {
        BR_PROFILE_FUNCTION();
        std::ifstream j(p);

        std::string fullFile;
        std::string line;

        while (std::getline(j, line)) {
            fullFile += line;
        }

        j.close();
        return nlohmann::json::parse(fullFile);
    };

    auto json = loadJson(path);
    // The keys are currently hardcoded. This isn't a problem as long as we stick to the same scheme.
    // However, adding a way to configure these fields might be a good idea.
    return OverallTestResult {
      .SerialNumber = json.at("info").at("serial").get<std::string>(),
      .Duration     = json.at("info").at("time").at("process").get<double>(),
      .Date         = json.at("info").at("date").get<std::string>(),
      .Passed       = json.at("info").at("pass").get<bool>(),
      .Version      = json.at("info").at("version").at("scripts").get<std::string>(),
      .Uut          = json.at("info").at("uut").get<int>(),
      .Sequences    = LoadSequences(json.at("sequences")),
    };
}

std::map<std::string, ResultViewer::SequenceResult> ResultViewer::LoadSequences(const nlohmann::json& sequences)
{
    std::map<std::string, SequenceResult> sequenceResults = {};
    for (const auto& [seqName, seqDetails] : sequences.items()) {
        sequenceResults[seqName] = SequenceResult {
          .Name     = seqName,
          .Duration = seqDetails.at("time").at("process").get<double>(),
          .Enabled  = seqDetails.at("enabled").get<bool>(),
          .Skipped  = seqDetails.at("skipped").get<bool>(),
          .Passed   = seqDetails.at("pass").get<bool>(),
          .Tests    = LoadTests(seqDetails.at("tests")),
        };
    }
    return sequenceResults;
}

std::map<std::string, ResultViewer::TestResult> ResultViewer::LoadTests(const nlohmann::json& tests)
{
    std::map<std::string, TestResult> testResults = {};

    for (const auto& [testName, testDetails] : tests.items()) {
        testResults[testName] = TestResult {
          .Name         = testName,
          .Duration     = testDetails.at("time").at("process").get<double>(),
          .Enabled      = testDetails.at("enabled").get<bool>(),
          .Skipped      = testDetails.at("skipped").get<bool>(),
          .Passed       = testDetails.at("pass").get<bool>(),
          .Expectations = LoadExpectations(testDetails.at("expectations")),
        };
    }

    return testResults;
}
std::vector<ResultViewer::ExpectationDetails> ResultViewer::LoadExpectations(const nlohmann::json& expectations)
{
    std::vector<ExpectationDetails> expectationDetails = {};

    for (const auto& [expName, expDetails] : expectations.items()) {
        ExpectationDetails details;
        for (const auto& [fieldName, fieldVal] : expDetails.items()) {
            details[fieldName] = fieldVal;
        }
        expectationDetails.push_back(details);
    }

    return expectationDetails;
}

std::string ResultViewer::MakeStringFromJson(const std::string& key, const nlohmann::json& value)
{
    if (value.is_number_float()) { return std::format("{}: {:0.6f}", key, value.get<float>()); }
    return std::format("{}: {}", key, value.dump(4, ' ', true));
}

}    // namespace Frasy
