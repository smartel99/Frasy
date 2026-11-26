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
    std::vector<std::string> files;
    // Incomplete boards are in debug.
    for (const auto* folder : {"fail", "pass", "debug"}) {
        auto found = LoadMatchingFiles(std::format("logs/{}/{}", title, folder), serialNumbers);
        files.insert(files.end(), found.begin(), found.end());
    }
    return files;
}
/**
 * @brief Read every scope's result, duration and children of a report.
 * Done before anything is counted, so a report with a bad scope isn't partly counted.
 * @throws Report::ReportFormatError or nlohmann::json::exception on the first unreadable scope.
 */
void validateReport(const Report::ResultReader& reader, const nlohmann::json& report)
{
    (void)report.at("info").at("time").at("process").get<double>();
    for (const auto& sequence : report.at("sequences")) {
        if (!reader.isScope(sequence)) { continue; }
        const auto result = reader.read(sequence);
        (void)sequence.at("time").at("process").get<double>();
        for (const auto& test : sequence.at("tests")) {
            if (!reader.isScope(test)) { continue; }
            (void)reader.read(test, result);
            (void)test.at("time").at("process").get<double>();
            if (!test.at("expectations").is_structured()) {
                throw Report::ReportFormatError("Test expectations aren't a list");
            }
        }
    }
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
        catch (const std::exception& e) {
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
        const Report::ResultReader reader(file);
        validateReport(reader, file);
        auto& locationResults = m_results.Locations[location];
        locationResults.Name  = location;
        locationResults.Results.add(reader.getInfo());

        locationResults.Durations.push_back(info.at("time").at("process").get<double>());
        locationResults.AverageDuration =
          std::accumulate(locationResults.Durations.begin(), locationResults.Durations.end(), 0.0);
        locationResults.AverageDuration /= static_cast<double>(locationResults.Durations.size());

        auto sequences = file.at("sequences");
        for (auto [name, sequence] : sequences.items()) {
            if (!reader.isScope(sequence)) { continue; }
            if (m_options.Sequences.empty() ||
                std::ranges::any_of(m_options.Sequences, [&name](const auto& sequenceName) {
                    return std::string_view(sequenceName.data()) == name;
                })) {
                BR_LOG_DEBUG("Analyzer", "Analyzing sequence '{}'...", name);
                AnalyzeSequence(reader, sequence, locationResults.Sequences[name]);
                locationResults.Sequences[name].Name = name;
            }
            else {
                BR_LOG_DEBUG("Analyzer", "Ignored sequence '{}'...", name);
            }
        }
    }
}

void ResultAnalyzer::AnalyzeSequence(const Report::ResultReader&      reader,
                                     const nlohmann::json&            sequence,
                                     ResultAnalysisResults::Sequence& results)
{
    const auto sequenceResult = reader.read(sequence);
    results.Results.add(sequenceResult);

    results.Durations.push_back(sequence.at("time").at("process").get<double>());
    results.AverageDuration = std::accumulate(results.Durations.begin(), results.Durations.end(), 0.0);
    results.AverageDuration /= static_cast<double>(results.Durations.size());

    auto tests = sequence.at("tests");
    for (auto [name, test] : tests.items()) {
        if (!reader.isScope(test)) { continue; }
        if (m_options.Tests.empty() || std::ranges::any_of(m_options.Tests, [&name](const auto& testName) {
                return std::string_view(testName.data()) == name;
            })) {
            BR_LOG_DEBUG("Analyzer", "Analyzing test '{}'...", name);
            AnalyzeTest(reader, test, sequenceResult, results.Tests[name]);
            results.Tests[name].Name = name;
        }
        else {
            BR_LOG_DEBUG("Analyzer", "Ignored test '{}'...", name);
        }
    }
}

void ResultAnalyzer::AnalyzeTest(const Report::ResultReader&  reader,
                                 const nlohmann::json&        test,
                                 Report::Result               sequenceResult,
                                 ResultAnalysisResults::Test& results)
{
    const auto result = reader.read(test, sequenceResult);
    results.Results.add(result);

    // Tests that didn't run have no duration to average. A test that never started has `time.stop == 0`, e.g. one of
    // a sequence whose body raised.
    const auto& time     = test.at("time");
    const auto  stop     = time.find("stop");
    const bool  neverRan = stop != time.end() && stop->is_number() && stop->get<double>() == 0.0;
    if (result != Report::Result::Skipped && result != Report::Result::Disabled && !neverRan) {
        results.Durations.push_back(time.at("process").get<double>());
        results.AverageDuration = std::accumulate(results.Durations.begin(), results.Durations.end(), 0.0);
        results.AverageDuration /= static_cast<double>(results.Durations.size());
    }

    auto expectations = test.at("expectations");
    for (auto expectation : expectations) {
        std::string name = std::format("Expectation {}", results.Expectations.size() + 1);
        if (expectation.contains("name")) { name = expectation.at("name").get<std::string>(); }
        try {
            // Read first: an expectation created without a value has no stats to make.
            const bool passed = reader.read(expectation) == Report::Result::Pass;
            if (!results.Expectations.contains(name)) {
                results.Expectations[name] = std::move(MakeExpectationFromDetails(expectation));
            }
            results.Expectations[name]->AddValue(expectation, passed);
        }
        catch (std::exception& e) {
            BR_LOG_ERROR("Analyzer", "Error while analyzing expectation '{}': {}", name, e.what());
        }
    }
}

std::shared_ptr<ResultAnalysisResults::Expectation> ResultAnalyzer::MakeExpectationFromDetails(
  const nlohmann::json& expectation)
{
    using namespace std::string_literals;

    std::string method = expectation.at("method").get<std::string>();

    std::shared_ptr<ResultAnalysisResults::Expectation> exp;

    if (method == "ToBeTrue"s) { exp = std::make_shared<ToBeTrueExpectation>(); }
    else if (method == "ToBeFalse"s) {
        exp = std::make_shared<ToBeFalseExpectation>();
    }
    else if (method == "ToBeEqual"s) {
        exp = std::make_shared<ToBeEqualExpectation>(expectation.at("expected"));
    }
    else if (method == "ToBeNear"s) {
        exp = std::make_shared<ToBeNearExpectation>(expectation.at("expected").get<float>(),
                                                    expectation.at("deviation").get<float>());
    }
    else if (method == "ToBeInRange"s) {
        exp = std::make_shared<ToBeInRangeExpectation>(expectation.at("min").get<float>(),
                                                       expectation.at("max").get<float>());
    }
    else if (method == "ToBeInPercentage"s) {
        exp = std::make_shared<ToBeInPercentageExpectation>(expectation.at("expected").get<float>(),
                                                            expectation.at("percentage").get<float>());
    }
    else if (method == "ToBeGreater"s) {
        exp = std::make_shared<ToBeGreaterExpectation>(expectation.at("min").get<float>());
    }
    else if (method == "ToBeGreaterOrEqual"s) {
        exp = std::make_shared<ToBeGreaterOrEqualExpectation>(expectation.at("min").get<float>());
    }
    else if (method == "ToBeLesser"s) {
        exp = std::make_shared<ToBeLesserExpectation>(expectation.at("max").get<float>());
    }
    else if (method == "ToBeLesserOrEqual"s) {
        exp = std::make_shared<ToBeLesserOrEqualExpectation>(expectation.at("max").get<float>());
    }
    else if (method == "ToBeType"s) {
        exp = std::make_shared<ToBeTypeExpectation>(expectation.at("expected").get<std::string>());
    }
    else if (method == "ToMatch"s) {
        BR_LOG_WARN("Analyzer", "ToMatch is not supported yet");
        throw std::runtime_error(std::format("Unsupported method {}", method));
    }
    else {
        throw std::runtime_error(std::format("Invalid method {}", method));
    }

    std::string name = expectation.at("name").get<std::string>();
    std::string note = expectation.at("note").get<std::string>();
    exp->Name        = name == note ? name : std::format("{} - {}", name, note);
    return exp;
}
}    // namespace Frasy::Analyzers
