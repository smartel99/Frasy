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

#include "formatters/keyValue.h"
#include "utils/solution_loader.h"
#include <Brigerad/Core/Log.h>
#include <json.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>


namespace Frasy::Report::Kvp {
// namespace

std::vector<std::string> makeReport(const sol::table& results, const std::vector<std::string>& filenames)
{
    static constexpr auto    s_tag   = "KVP Report";
    const auto               lua     = sol::state_view(results.lua_state());
    std::vector<std::string> reports = {};
    // Load the solution up, building a list of the sections-solutions-tests-expectations
    // Format them from the results into a kvp format, where keys are <solution>#<test>#<expectation>
    try {
        using namespace Frasy::Report::SolutionLoader;
        Solution solution = loadSolution();

        auto title = results["info"]["title"].get<std::string>();
        auto pass  = results["info"]["pass"].get<bool>();

        namespace fs = std::filesystem;
        // Create log directory if needed.
        static auto frasyLogDirectory = fs::current_path() / ".."/ "logs";
        create_directories(frasyLogDirectory);
        auto logDirectory = frasyLogDirectory / title;
        create_directories(logDirectory);

        // Create SMT log directly if needed.
        auto smtDirectory = logDirectory / "smt";
        create_directories(smtDirectory);
        auto smtPassDir = smtDirectory / "pass";
        create_directories(smtPassDir);
        auto smtFailDir = smtDirectory / "fail";
        create_directories(smtFailDir);

        // Create the log file.
        auto smtFileDir         = pass ? smtPassDir : smtFailDir;
        auto lastReportFilepath = smtDirectory / fmt::format("last_uut{}.txt", results.get<sol::table>("info").get_or("uut", 0));

        std::ofstream report(lastReportFilepath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!report.is_open()) {
            BR_LOG_ERROR(s_tag, "Unable to open file '{}'", lastReportFilepath.string());
            return {};
        }

        static constexpr auto endline   = "\n";
        auto                  formatter = Formatter::KeyValue(report, results);

        formatter.reportInfo();
        formatter.reportUserInfo(lua["Context"]["map"]["onReportInfo"]());
        report << endline;

        for (const auto& [name, ib] : results["ib"].get<sol::table>()) {
            formatter.reportIb(name.as<std::string>());
        }
        report << endline;
        report << "---\n\n";

        for (const auto& [seqName, sequence] : solution) {
            formatter.reportSequenceResult(seqName);
            report << endline;
            for (const auto& test : sequence) {
                formatter.reportTestResult(test);
                for (sol::table expectation = formatter.getNextExpectation(); !expectation.empty();
                     expectation            = formatter.getNextExpectation()) {
                    formatter.reportExpectationResult(expectation);
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
        BR_LOG_ERROR(s_tag, "Error while making report: {}", e.what());
    }
    return reports;
}
}    // namespace Frasy::Report::Kvp