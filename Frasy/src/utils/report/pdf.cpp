/**
 * @file    table.cpp
 * @author  Sam Martel
 * @date    2026-02-02
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
#include "pdf.h"

#include "formatters/pdf.h"
#include "utils/solution_loader.h"
#include <Brigerad/Core/Log.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace Frasy::Report::PDF {
std::vector<std::string> makeReport(const sol::table& results, const std::vector<std::string>& filenames)
{
    static constexpr auto    s_tag   = "Table Report";
    const auto               lua     = sol::state_view(results.lua_state());
    std::vector<std::string> reports = {};
    try {

        using namespace Frasy::Report::SolutionLoader;
        Solution solution = loadSolution();

        auto title = results["info"]["title"].get<std::string>();
        auto pass  = results["info"]["pass"].get<bool>();

        namespace fs = std::filesystem;
        // Create log directory if needed.
        static auto frasyLogDirectory = fs::current_path() / ".." / "logs";
        create_directories(frasyLogDirectory);
        auto logDirectory = frasyLogDirectory / title;
        create_directories(logDirectory);

        // Create client log directly if needed.
        auto clientDirectory = logDirectory / "client";
        create_directories(clientDirectory);
        auto clientPassDir = clientDirectory / "pass";
        create_directories(clientPassDir);
        auto clientFailDir = clientDirectory / "fail";
        create_directories(clientFailDir);

        // Create the log file.
        auto clientFileDir = pass ? clientPassDir : clientFailDir;
        auto lastReportFilepath =
          clientDirectory / fmt::format("last_uut{}.pdf", results.get<sol::table>("info").get_or("uut", 0));
        auto formatter = Formatter::PDF(lastReportFilepath.string(), "Frasy Report", results);

        formatter.reportInfo();
        if (lua["Context"]["map"]["onReportInfo"].get_type() != sol::type::nil) {
            BR_LOG_ERROR(s_tag, "reportUserInfo not implemented for PDF output!");
        }

        formatter.startReportIb();
        for (const auto& [name, ib] : results["ib"].get<sol::table>()) {
            formatter.reportIb(name.as<std::string>());
        }
        formatter.endReportIb();

        formatter.startReportSequences();

        for (const auto& [seqName, sequence] : solution) {
            formatter.reportSequenceResult(seqName);
            for (const auto& test : sequence) {
                formatter.reportTestResult(test);
                for (sol::table expectation = formatter.getNextExpectation(); !expectation.empty();
                     expectation            = formatter.getNextExpectation()) {
                    formatter.reportExpectationResult(expectation);
                }
                formatter.endReportTest();
            }
            formatter.endReportSequence();
        }

        formatter.convert();
        reports.emplace_back(lastReportFilepath.string());
        for (const auto& filename : filenames) {
            const auto filepath = clientFileDir / filename;
            reports.emplace_back(filepath.string());
            copy(lastReportFilepath, filepath, fs::copy_options::overwrite_existing);
        }
    }
    catch (const std::exception& e) {
        BR_LOG_ERROR(s_tag, "Error while making report: {}", e.what());
    }
    return reports;
}
}    // namespace Frasy::Report::PDF
