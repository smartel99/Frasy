/**
 * @file    analyzer.cpp
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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "analyzer.h"

#include "analytic_results.h"
#include "expectations/to_be_equal.h"
#include "expectations/to_be_false.h"
#include "expectations/to_be_greater.h"
#include "expectations/to_be_greater_or_equal.h"
#include "expectations/to_be_in_percentage.h"
#include "expectations/to_be_in_range.h"
#include "expectations/to_be_lesser.h"
#include "expectations/to_be_lesser_or_equal.h"
#include "expectations/to_be_near.h"
#include "expectations/to_be_true.h"
#include "expectations/to_be_type.h"

#include <Brigerad.h>
#include <Brigerad/Debug/Instrumentor.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <json.hpp>
#include <string_view>

namespace Frasy::Analyzers {
namespace {
float Percent(size_t v, size_t tot)
{
    return (static_cast<float>(v) / static_cast<float>(tot)) * 100.0f;
}

nlohmann::json LoadJson(const std::string& path)
{
    BR_PROFILE_FUNCTION();
    // Open the file or create it if it doesn't exist.
    std::fstream file(path, std::ios::out | std::ios::app);
    file.close();
    std::ifstream j(path);

    std::string fullFile;
    std::string line;

    while (std::getline(j, line)) {
        fullFile += line;
    }
    j.close();

    try {
        return nlohmann::json::parse(fullFile);
    }
    catch (nlohmann::json::parse_error& e) {
        BR_APP_ERROR("An error occurred while parsing '{}': {}", path, e.what());
        return {};
    }
}

std::vector<std::string> LoadMatchingFiles(const std::string&                       path,
                                           const std::vector<std::array<char, 32>>& serialNumbers)
{
    namespace fs = std::filesystem;
    auto numbers = [&serialNumbers]() {
        std::vector<std::string_view> numbers = {};
        numbers.reserve(serialNumbers.size());

        for (auto&& number : serialNumbers) {
            numbers.emplace_back(number.data());
        }

        return numbers;
    }();

    std::vector<std::string> files = {};

    try {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (serialNumbers.empty() ||
                std::ranges::any_of(numbers, [entryPath = entry.path().string()](const auto& number) {
                    return entryPath.find(number) != std::string::npos;
                })) {
                files.emplace_back(entry.path().string());
            }
        }
    }
    catch (std::filesystem::filesystem_error& e) {
        BR_LOG_ERROR("Analyzer", "{}", e.what());
    }
    return files;
}

std::vector<std::string> LoadAllMatchingFiles(const std::string&                       title,
                                              const std::vector<std::array<char, 32>>& serialNumbers)
{
    auto r   = LoadMatchingFiles(std::format("logs/{}/fail", title), serialNumbers);
    auto tmp = LoadMatchingFiles(std::format("logs/{}/pass", title), serialNumbers);
    r.insert(r.end(), tmp.begin(), tmp.end());
    return r;
}
}    // namespace


ResultAnalysisResults ResultAnalyzer::Analyze(const std::string& title)
{
    m_results = {};
    // Load all files that match the options from the pass and fail directory
    auto logs = LoadAllMatchingFiles(title, m_options.SerialNumbers);
    ToAnalyze = logs.size();
    // For each file found, ignore those with UUTs that do not interest us
    for (auto&& log : logs) {
        try {
            BR_LOG_DEBUG("Analyzer", "Analyzing log '{}'...", log);
            AnalyzeFile(log);
        }
        catch (nlohmann::json::exception& e) {
            BR_LOG_ERROR("Analyzer", "An error occurred while analyzing '{}': {}", log, e.what());
        }
        Analyzed++;
    }

    for (auto&& [name, location] : m_results.Locations) {
        for (auto&& [sName, sequence] : location.Sequences) {
            for (auto&& [tName, test] : sequence.Tests) {
                for (auto&& [eName, expectation] : test.Expectations) {
                    expectation->MakeStats();
                }
            }
        }
    }

    return m_results;
}

void ResultAnalyzer::AnalyzeFile(const std::string& path)
{
    auto file = LoadJson(path);

    const auto& info     = file.at("info");
    std::string location = info.at("uut").dump();

    // Check if this log is one that interests us based on its location.
    if (m_options.Uuts.empty() || std::ranges::any_of(m_options.Uuts, [&location](const std::array<char, 32>& u) {
            return location.find(std::string_view(u.data())) != std::string::npos;
        })) {
        // File is of interest, proceed with the analysis.
        if (m_options.Ganged) { location = "Total"; }
        else {
            location = std::format("Location {}", info.at("uut").get<int>());
        }
        auto& locationResults = m_results.Locations[location];
        locationResults.Name  = location;
        locationResults.Total++;
        if (info.at("pass").get<bool>()) { locationResults.Passed++; }
        locationResults.PassedPercent = Percent(locationResults.Passed, locationResults.Total);

        locationResults.Durations.push_back(info.at("time").at("elapsed").get<double>());
        locationResults.AverageDuration =
          std::accumulate(locationResults.Durations.begin(), locationResults.Durations.end(), 0.0);
        locationResults.AverageDuration /= static_cast<double>(locationResults.Durations.size());

        auto sequences = file.at("sequences");
        for (auto [name, sequence] : sequences.items()) {
            if (m_options.Sequences.empty() ||
                std::ranges::any_of(m_options.Sequences, [&name](const auto& sequenceName) {
                    return std::string_view(sequenceName.data()) == name;
                })) {
                BR_LOG_DEBUG("Analyzer", "Analyzing sequence '{}'...", name);
                AnalyzeSequence(sequence, locationResults.Sequences[name]);
            }
            else {
                BR_LOG_DEBUG("Analyzer", "Ignored sequence '{}'...", name);
            }
        }
    }
}

void ResultAnalyzer::AnalyzeSequence(const nlohmann::json& sequence, ResultAnalysisResults::Sequence& results)
{
    results.Total++;
    if (sequence.at("enabled").get<bool>()) { results.Enabled++; }
    results.EnabledPercent = Percent(results.Enabled, results.Total);
    if (sequence.at("skipped").get<bool>()) { results.Skipped++; }
    results.SkippedPercent = Percent(results.Skipped, results.Total);
    if (sequence.at("pass").get<bool>()) { results.Passed++; }
    results.PassedPercent = Percent(results.Passed, results.Total - results.Skipped);

    results.Durations.push_back(sequence.at("time").at("elapsed").get<double>());
    results.AverageDuration = std::accumulate(results.Durations.begin(), results.Durations.end(), 0.0);
    results.AverageDuration /= static_cast<double>(results.Durations.size());

    auto tests = sequence.at("tests");
    for (auto [name, test] : tests.items()) {
        if (m_options.Tests.empty() || std::ranges::any_of(m_options.Tests, [&name](const auto& testName) {
                return std::string_view(testName.data()) == name;
            })) {
            BR_LOG_DEBUG("Analyzer", "Analyzing test '{}'...", name);
            AnalyzeTest(test, results.Tests[name]);
        }
        else {
            BR_LOG_DEBUG("Analyzer", "Ignored test '{}'...", name);
        }
    }
}

void ResultAnalyzer::AnalyzeTest(const nlohmann::json& test, ResultAnalysisResults::Test& results)
{
    results.Total++;
    if (test.at("enabled").get<bool>()) { results.Enabled++; }
    results.EnabledPercent = Percent(results.Enabled, results.Total);
    if (test.at("skipped").get<bool>()) { results.Skipped++; }
    results.SkippedPercent = Percent(results.Skipped, results.Total);
    if (test.at("pass").get<bool>()) { results.Passed++; }
    results.PassedPercent = Percent(results.Passed, results.Total - results.Skipped);

    results.Durations.push_back(test.at("time").at("elapsed").get<double>());
    results.AverageDuration = std::accumulate(results.Durations.begin(), results.Durations.end(), 0.0);
    results.AverageDuration /= static_cast<double>(results.Durations.size());

    auto expectations = test.at("expectations");
    for (auto expectation : expectations) {
        std::string name = std::format("Expectation {}", results.Expectations.size() + 1);
        if (expectation.contains("name")) { name = expectation.at("name").get<std::string>(); }
        try {
            if (!results.Expectations.contains(name)) {
                results.Expectations[name] = std::move(MakeExpectationFromDetails(expectation));
            }
            AnalyzeExpectation(expectation, results.Expectations[name]);
        }
        catch (std::exception& e) {
            BR_LOG_ERROR("Analyzer", "Error while analyzing expectation '{}': {}", name, e.what());
        }
    }
}

void ResultAnalyzer::AnalyzeExpectation(const nlohmann::json&                                expectation,
                                        std::shared_ptr<ResultAnalysisResults::Expectation>& results)
{
    results->AddValue(expectation);
}

std::shared_ptr<ResultAnalysisResults::Expectation> ResultAnalyzer::MakeExpectationFromDetails(
  const nlohmann::json& expectation)
{
    using namespace std::string_literals;

    std::string method = expectation.at("method").get<std::string>();

    if (method == "ToBeTrue"s) {
        ToBeTrueExpectation* exp = new ToBeTrueExpectation();
        return std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    }
    if (method == "ToBeFalse"s) {
        ToBeFalseExpectation* exp = new ToBeFalseExpectation();
        return std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    }
    if (method == "ToBeEqual"s) {
        ToBeEqualExpectation* exp = new ToBeEqualExpectation(expectation.at("expected"));
        return std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    }
    if (method == "ToBeNear"s) {
        ToBeNearExpectation* exp =
          new ToBeNearExpectation(expectation.at("expected").get<float>(), expectation.at("deviation").get<float>());
        return std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    }
    if (method == "ToBeInRange"s) {
        ToBeInRangeExpectation* exp =
          new ToBeInRangeExpectation(expectation.at("min").get<float>(), expectation.at("max").get<float>());
        return std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    }
    if (method == "ToBeInPercentage"s) {
        ToBeInPercentageExpectation* exp = new ToBeInPercentageExpectation(expectation.at("expected").get<float>(),
                                                                           expectation.at("percentage").get<float>());
        return std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    }
    if (method == "ToBeGreater"s) {
        ToBeGreaterExpectation* exp = new ToBeGreaterExpectation(expectation.at("min").get<float>());
        return std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    }
    if (method == "ToBeGreaterOrEqual"s) {
        ToBeGreaterOrEqualExpectation* exp = new ToBeGreaterOrEqualExpectation(expectation.at("min").get<float>());
        return std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    }
    if (method == "ToBeLesser"s) {
        ToBeLesserExpectation* exp = new ToBeLesserExpectation(expectation.at("max").get<float>());
        return std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    }
    if (method == "ToBeLesserOrEqual"s) {
        ToBeLesserOrEqualExpectation* exp = new ToBeLesserOrEqualExpectation(expectation.at("max").get<float>());
        return std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    }
    if (method == "ToBeType"s) {
        ToBeTypeExpectation* exp = new ToBeTypeExpectation(expectation.at("expected").get<std::string>());
        return std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    }
    if (method == "ToMatch"s) { BR_LOG_WARN("Analyzer", "ToMatch is not supported yet"); }
    throw std::runtime_error(std::format("Invalid method {}", method));
}
}    // namespace Frasy::Analyzers
