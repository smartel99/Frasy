/**
 * @file    json.cpp
 * @author  Paul Thomas
 * @date    3/17/2025
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

#include "./json.h"
#include "formatters/json.h"

#include "formatters/keyValue.h"
#include "utils/solution_loader.h"
#include <Brigerad/Core/Log.h>
#include <filesystem>
#include <fstream>

namespace Frasy::Report::Json {
std::vector<std::string> makeReport(const sol::table& results, const std::vector<std::string>& filenames)
{
    static constexpr auto    s_tag   = "JSON Report";
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
        static auto frasyLogDirectory = fs::current_path() / "logs";
        create_directories(frasyLogDirectory);
        auto logDirectory = frasyLogDirectory / title;
        create_directories(logDirectory);

        // Create json log directly if needed.
        auto jsonDirectory = logDirectory / "json";
        create_directories(jsonDirectory);
        auto jsonPassDir = jsonDirectory / "pass";
        create_directories(jsonPassDir);
        auto jsonFailDir = jsonDirectory / "fail";
        create_directories(jsonFailDir);

        // Create the log file.
        auto jsonFileDir = pass ? jsonPassDir : jsonFailDir;
        auto lastReportFilepath =
          jsonDirectory / fmt::format("last_uut{}.json", results.get<sol::table>("info").get_or("uut", 0));

        auto formatter = Formatter::Json(results);

        formatter.reportInfo();
        formatter.reportUserInfo(lua["Context"]["map"]["onReportInfo"]());

        for (const auto& [name, ib] : results["ib"].get<sol::table>()) {
            formatter.reportIb(name.as<std::string>());
        }

        for (const auto& [seqName, sequence] : solution) {
            formatter.reportSequenceResult(seqName);
            for (const auto& test : sequence) {
                formatter.reportTestResult(test);
                for (sol::table expectation = formatter.getNextExpectation(); !expectation.empty();
                     expectation            = formatter.getNextExpectation()) {
                    formatter.reportExpectationResult(expectation);
                }
            }
        }

        formatter.toFile(lastReportFilepath.string());
        reports.emplace_back(lastReportFilepath.string());
        for (const auto& filename : filenames) {
            const auto filepath = jsonFileDir / filename;
            reports.emplace_back(filepath.string());
            copy(lastReportFilepath, filepath, fs::copy_options::overwrite_existing);
        }
    }
    catch (const std::exception& e) {
        BR_LOG_ERROR(s_tag, "Error while making report: {}", e.what());
    }
    return reports;
}
}    // namespace Frasy::Report::Json