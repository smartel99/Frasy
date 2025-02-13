/**
 * @file    kvp.cpp
 * @author  Samuel Martel
 * @date    2024-12-05
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
#include "kvp.h"

#include <Brigerad/Core/Log.h>
#include <json.hpp>
#include <utils/report_formatter/keyValue.h>

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <string>
#include <vector>


namespace Frasy::Report::Formatter::Kvp {
namespace {
using Test     = std::string;
using Sequence = std::pair<std::string, std::vector<Test>>;
using Solution = std::vector<Sequence>;

nlohmann::json loadJson(const std::string& path)
{
    if (!std::filesystem::exists(path)) { return {}; }
    std::ifstream  ifs(path);
    nlohmann::json jObject {};
    if (ifs.is_open()) {
        ifs >> jObject;
        ifs.close();
    }
    else {
        BR_LOG_ERROR("KVP Report", "Unable to open JSON file '{}'", path);
    }
    return jObject;
}

Solution loadSolution()
{
    auto     json = loadJson("lua/solution.json");
    Solution solution;
    if (json.empty()) { return solution; }

    for (const auto& section : json) {
        for (const auto& sequenceList : section) {
            for (const auto& sequence : sequenceList) {
                BR_LOG_DEBUG("KVP Report", "Loading sequence '{}'", sequence["name"]);
                for (const auto& testGroup : sequence["tests"]) {
                    for (const auto& test : testGroup) {
                        BR_LOG_DEBUG("KVP Report", "Test '{}'", test);
                        auto it = std::ranges::find_if(solution,
                                                       [name = sequence["name"].get<std::string>()](const auto& test) {
                                                           return test.first == name;
                                                       });
                        if (it != solution.end()) { it->second.push_back(test); }
                        else {
                            solution.emplace_back(sequence["name"], std::vector {test.get<std::string>()});
                        }
                    }
                }
            }
        }
    }

    return solution;
}
}    // namespace

std::vector<std::string> makeReport(sol::state_view&                lua,
                                    const sol::table&               results,
                                    const std::vector<std::string>& filenames)
{
    std::vector<std::string> reports = {};
    // Load the solution up, building a list of the sections-solutions-tests-expectations
    // Format them from the results into a kvp format, where keys are <solution>#<test>#<expectation>
    try {
        Solution solution = loadSolution();

        auto pass = results["info"]["pass"].get<bool>();

        namespace fs = std::filesystem;
        // Create log directory if needed.
        static auto logDirectory = fs::current_path() / "logs";
        create_directories(logDirectory);

        // Create SMT log directly if needed.
        static auto smtDirectory = logDirectory / "smt";
        create_directories(smtDirectory);
        static auto smtPassDir = smtDirectory / "pass";
        create_directories(smtPassDir);
        static auto smtFailDir = smtDirectory / "fail";
        create_directories(smtFailDir);

        // Create the log file.
        auto        smtFileDir         = pass ? smtPassDir : smtFailDir;
        static auto lastReportFilepath = smtDirectory / "last.txt";

        std::ofstream report(lastReportFilepath);
        if (!report.is_open()) {
            BR_LOG_ERROR("KVP Report", "Unable to open file '{}'", lastReportFilepath.string());
            return {};
        }

        static constexpr auto endline   = "\n";
        KeyValue              formatter = KeyValue(lua, report, results);

        formatter.reportInfo();
        report << endline;

        formatter.reportVersion();
        for (const auto& [name, ib] : results["ib"].get<sol::table>()) {
            formatter.reportIb(name.as<std::string>());
        }
        report << endline;

        sol::table infos = lua["Context"]["map"]["onReportInfo"]();
        for (const auto& info : infos) {
            report << info.second.as<std::string>() << KeyValue::endline;
        }
        report << "---\n\n";

        for (const auto& [seqName, sequence] : solution) {
            formatter.reportSequenceResult(seqName);
            report << endline;
            for (const auto& test : sequence) {
                formatter.reportTestResult(test);
                for (sol::table expectation = formatter.getNextExpectation(); !expectation.empty();
                     expectation            = formatter.getNextExpectation()) {
                    std::string kind = expectation["method"].get<std::string>();
                    using namespace std::string_view_literals;
                    if (kind == "ToBeTrue"sv || kind == "ToBeFalse"sv) {
                        formatter.reportToBeEqualBoolean(expectation);
                    }
                    else if (kind == "ToBeEqual"sv) {
                        sol::type type = expectation["value"].get_type();
                        if (type == sol::type::boolean) { formatter.reportToBeEqualBoolean(expectation); }
                        else if (type == sol::type::number) {
                            formatter.reportToBeEqualNumber(expectation);
                        }
                        else if (type == sol::type::string) {
                            formatter.reportToBeEqualString(expectation);
                        }
                    }
                    else if (kind == "ToBeNear"sv) {
                        formatter.reportToBeNear(expectation);
                    }
                    else if (kind == "ToBeInRange"sv) {
                        formatter.reportToBeInRange(expectation);
                    }
                    else if (kind == "ToBeInPercentage"sv) {
                        formatter.reportToBeInPercentage(expectation);
                    }
                    else if (kind == "ToBeGreaterOrEqual"sv || kind == "ToBeGreater"sv) {
                        formatter.reportToBeGreater(expectation);
                    }
                    else if (kind == "ToBeLesserOrEqual"sv || kind == "ToBeLesser"sv) {
                        formatter.reportToBeLesser(expectation);
                    }
                    report << endline;
                }
            }
        }

        report.close();
        reports.emplace_back(lastReportFilepath.string());
        for (const auto& filename : filenames) {
            const auto filepath = smtFileDir / filename;
            reports.emplace_back(filepath.string());
            copy(lastReportFilepath, filepath, fs::copy_options::overwrite_existing);
        }
    }
    catch (const std::exception& e) {
        BR_LOG_ERROR("KVP Report", "Error while making report: {}", e.what());
    }
    return reports;
}
}    // namespace Frasy::Report::Formatter::Kvp
