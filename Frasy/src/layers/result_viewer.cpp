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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "result_viewer.h"

#include "imgui.h"
#include "utils/report/formatters/formatter.h"

namespace Frasy {

using Report::Result;

ResultViewer::ResultViewer() noexcept : m_logs(LoadLogs(true))
{
    m_isFirstPassOfLogs.resize(m_logs.size());
    std::ranges::fill(m_isFirstPassOfLogs, true);
}

void ResultViewer::onImGuiRender()
{
    if (!m_isVisible) { return; }

    if (ImGui::Begin(s_windowName, &m_isVisible, ImGuiWindowFlags_NoDocking)) {
        if (AreLogsNew()) {
            if (m_isFirstPassOfLogs.size() != m_logs.size()) { m_isFirstPassOfLogs.resize(m_logs.size()); }
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
            std::ranges::fill(m_isFirstPassOfLogs, true);
        }

        if (ImGui::BeginTabBar("ResultTabBar",
                               ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll)) {
            for (std::size_t i = 0; i < m_logs.size(); ++i) {
                auto&&     log   = m_logs[i];
                const auto color = getTabColor(log.Results.Result, log.Results.HasFailure);
                if (color) { ImGui::PushStyleColor(ImGuiCol_Text, *color); }
                if (ImGui::BeginTabItem(log.Name.c_str())) {
                    if (color) { ImGui::PopStyleColor(); }
                    if (ImGui::BeginChild(log.Name.c_str(), ImVec2 {0.0f, 0.0f}, false)) {
                        if (log.IsGood) { RenderLog(log.Results, i); }
                        else {
                            ImGui::Text("Log is not valid");
                        }
                    }
                    ImGui::EndChild();

                    ImGui::EndTabItem();
                    m_isFirstPassOfLogs[i] = false;
                }
                else {
                    if (color) { ImGui::PopStyleColor(); }
                }
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void ResultViewer::RenderLog(const OverallTestResult& log, std::size_t index)
{
    const auto label = Report::Formatter::toLabel(log.Result);
    ImGui::Text("Serial Number: %s, UUT %d - %s%s",
                log.SerialNumber.c_str(),
                log.Uut,
                label.c_str(),
                log.Result == Result::Incomplete && log.HasFailure ? " (a test failed)" : "");
    ImGui::Text("Date: %s, Duration: %0.3f seconds", log.Date.c_str(), log.Duration);
    ImGui::Text("Version: %s", log.Version.c_str());

    for (auto&& [sequenceName, sequence] : log.Sequences) {
        if (m_isFirstPassOfLogs[index]) { ImGui::SetNextItemOpen(isOpenByDefault(sequence.Result), ImGuiCond_Always); }
        if (ImGui::TreeNode(sequenceName.c_str())) {
            RenderSequence(sequence, index);
            ImGui::TreePop();
        }
    }
}

void ResultViewer::RenderSequence(const SequenceResult& sequence, std::size_t index)
{
    ImGui::Text("Name: %s - %s", sequence.Name.c_str(), Report::Formatter::toLabel(sequence.Result).c_str());
    ImGui::Text("Duration: %0.3f seconds", sequence.Duration);

    for (auto&& [testName, test] : sequence.Tests) {
        if (m_isFirstPassOfLogs[index]) { ImGui::SetNextItemOpen(isOpenByDefault(test.Result), ImGuiCond_Always); }
        if (ImGui::TreeNode(testName.c_str())) {
            RenderTest(test, index);
            ImGui::TreePop();
        }
        ImGui::Separator();
    }
}

void ResultViewer::RenderTest(const TestResult& test, std::size_t index)
{
    ImGui::Text("Name: %s - %s", test.Name.c_str(), Report::Formatter::toLabel(test.Result).c_str());
    ImGui::Text("Duration: %0.3f seconds", test.Duration);

    if (m_isFirstPassOfLogs[index]) { ImGui::SetNextItemOpen(isOpenByDefault(test.Result), ImGuiCond_Always); }
    if (ImGui::TreeNode(std::format("Expectations ({})", test.Expectations.size()).c_str())) {
        size_t at = 1;
        for (auto&& expectation : test.Expectations) {
            if (m_isFirstPassOfLogs[index]) {
                ImGui::SetNextItemOpen(isOpenByDefault(expectation.Result), ImGuiCond_Always);
            }
            const auto& details = expectation.Details;
            std::string label   = std::format("Expectation {}", at);
            if (details.contains("note")) {
                // Multi-line notes are stored as an array of lines by SanitizeExpectation, and only the
                // first one can fit on the node anyway.
                const auto& note = details.at("note");
                if (note.is_string()) { label = note.get<std::string>(); }
                else if (note.is_array() && !note.empty() && note.front().is_string()) {
                    label = std::format("{}...", note.front().get<std::string>());
                }
            }
            if (ImGui::TreeNode(&expectation, "%s", label.c_str())) {
                RenderExpectation(details);
                ImGui::TreePop();
            }
            at++;
        }
        ImGui::TreePop();
    }
}

void ResultViewer::RenderExpectation(const ResultViewer::ExpectationDetails& expectation)
{
    bool has_value = false;
    for (const auto& [key, value] : expectation) {
        if (key == "value") { has_value = true; }
        ImGui::TextWrapped("%s", MakeStringFromJson(key, value).c_str());
    }
    if (!has_value) { ImGui::TextWrapped("value: nil"); }
}

bool ResultViewer::isOpenByDefault(std::optional<Result> result)
{
    return result == Result::Fail || result == Result::Incomplete;
}

std::optional<uint32_t> ResultViewer::getTabColor(Result result, bool hasFailure)
{
    if (result == Result::Fail || (result == Result::Incomplete && hasFailure)) { return s_failColor; }
    if (result == Result::Incomplete) { return s_incompleteColor; }
    return std::nullopt;
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

    auto                       json = loadJson(path);
    const Report::ResultReader reader(json);
    // The keys are currently hardcoded. This isn't a problem as long as we stick to the same scheme.
    // However, adding a way to configure these fields might be a good idea.
    return OverallTestResult {
      .SerialNumber = json.at("info").at("serial").get<std::string>(),
      .Duration     = json.at("info").at("time").at("process").get<double>(),
      .Date         = json.at("info").at("date").get<std::string>(),
      .Result       = reader.getInfo(),
      .HasFailure   = Report::hasFailedScope(json),
      .Version      = json.at("info").at("version").at("scripts").get<std::string>(),
      .Uut          = json.at("info").at("uut").get<int>(),
      .Sequences    = LoadSequences(reader, json.at("sequences")),
    };
}

std::map<std::string, ResultViewer::SequenceResult> ResultViewer::LoadSequences(const Report::ResultReader& reader,
                                                                                const nlohmann::json&       sequences)
{
    std::map<std::string, SequenceResult> sequenceResults = {};
    for (const auto& [seqName, seqDetails] : sequences.items()) {
        if (!reader.isScope(seqDetails)) { continue; }
        const auto result        = reader.read(seqDetails);
        sequenceResults[seqName] = SequenceResult {
          .Name     = seqName,
          .Duration = seqDetails.at("time").at("process").get<double>(),
          .Result   = result,
          .Tests    = LoadTests(reader, seqDetails.at("tests"), result),
        };
    }
    return sequenceResults;
}

std::map<std::string, ResultViewer::TestResult> ResultViewer::LoadTests(const Report::ResultReader& reader,
                                                                        const nlohmann::json&       tests,
                                                                        Result                      sequenceResult)
{
    std::map<std::string, TestResult> testResults = {};

    for (const auto& [testName, testDetails] : tests.items()) {
        if (!reader.isScope(testDetails)) { continue; }
        testResults[testName] = TestResult {
          .Name         = testName,
          .Duration     = testDetails.at("time").at("process").get<double>(),
          .Result       = reader.read(testDetails, sequenceResult),
          .Expectations = LoadExpectations(reader, testDetails.at("expectations")),
        };
    }

    return testResults;
}

std::vector<ResultViewer::ExpectationResult> ResultViewer::LoadExpectations(const Report::ResultReader& reader,
                                                                            const nlohmann::json&       expectations)
{
    std::vector<ExpectationResult> expectationDetails = {};

    for (const auto& [expName, expDetails] : expectations.items()) {
        // Entries without a result, e.g. added by onReport, are still shown.
        ExpectationResult expectation;
        if (reader.isScope(expDetails)) { expectation.Result = reader.read(expDetails); }
        for (const auto& [fieldName, fieldVal] : expDetails.items()) {
            expectation.Details[fieldName] = SanitizeExpectation(fieldVal);
        }
        expectationDetails.push_back(std::move(expectation));
    }

    return expectationDetails;
}

nlohmann::json ResultViewer::SanitizeExpectation(const nlohmann::json& expectation)
{
    auto sanitize = [](this auto&& self, const nlohmann::json& field) -> nlohmann::json {
        nlohmann::json sanitized;
        if (field.is_object() || field.is_array()) {
            for (const auto& [fieldName, fieldVal] : field.items()) {
                sanitized[fieldName] = self(fieldVal);
            }
        }
        else if (field.is_string()) {
            auto                    sv                = field.get<std::string_view>();
            static constexpr size_t s_maxStringLength = 1024;
            std::string             str;
            if (sv.size() > s_maxStringLength) {
                str = std::format(
                  "{}... \n\n(truncated {} bytes)", sv.substr(0, s_maxStringLength), sv.size() - s_maxStringLength);
            }
            else {
                str = sv;
            }

            // Turn string into an array of strings split by lines so that it's prettier in the render.
            if (!str.contains('\n')) {
                // Single line, keep it as-is rather than wrapping it in a one-element array.
                sanitized = std::move(str);
            }
            else {
                sanitized = std::views::split(std::string_view {str}, '\n') | std::views::transform([](auto&& line) {
                                std::string_view sanitizedLine {line};
                                // Don't keep the carriage returns of CRLF line endings, they render as garbage.
                                if (sanitizedLine.ends_with('\r')) { sanitizedLine.remove_suffix(1); }
                                return std::string {sanitizedLine};
                            }) |
                            std::ranges::to<std::vector<std::string>>();
            }
        }
        else {
            return field;
        }
        return sanitized;
    };

    if (!expectation.is_object() && !expectation.is_array()) { return sanitize(expectation); }

    nlohmann::json exp;
    for (const auto& [fieldName, fieldVal] : expectation.items()) {
        exp[fieldName] = sanitize(fieldVal);
    }

    return exp;
}

std::string ResultViewer::MakeStringFromJson(const std::string& key, const nlohmann::json& value)
{
    if (value.is_number_float()) { return std::format("{}: {:0.6f}", key, value.get<float>()); }
    return std::format("{}: {}", key, value.dump(4, ' ', true));
}

}    // namespace Frasy
