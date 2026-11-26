/**
 * @file    uut_results.cpp
 * @author  Sam Martel
 * @date    2026-11-25
 * @brief   Results of a run as returned by the MCP `get_results` tool.
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
#include "uut_results.h"

#include "utils/report/result.h"

#include <Brigerad/Core/Log.h>

#include <algorithm>
#include <fstream>

namespace Frasy::Mcp {

nlohmann::json makeUutResult(std::size_t                  uut,
                             std::string_view             serial,
                             UutState                     state,
                             const std::filesystem::path& reportPath)
{
    nlohmann::json result;
    result["uut"]    = uut;
    result["serial"] = serial;
    result["pass"]   = state == UutState::Passed;
    result["state"]  = toString(state);
    if (const auto value = Report::getUutResult(state)) { result["result"] = Report::toString(*value); }

    Report::ResultCounts tests;
    if (std::filesystem::exists(reportPath)) {
        result["report_path"] = reportPath.generic_string();
        try {
            std::ifstream ifs(reportPath);
            const auto    report = nlohmann::json::parse(ifs);
            if (report.contains("info")) {
                result["duration"] = report["info"].value("time", nlohmann::json::object()).value("elapsed", 0.0);
            }
            tests = Report::countTests(report);
        }
        catch (const std::exception& e) {
            BR_LOG_ERROR("MCP", "Unable to read the tests of '{}': {}", reportPath.generic_string(), e.what());
        }
    }
    result["tests_passed"] = tests.get(Report::Result::Pass);
    result["tests_total"]  = tests.getTotal();
    result["test_results"] = tests.toJson();
    return result;
}

nlohmann::json makeRunResults(std::string_view product, std::span<const UutState> states, nlohmann::json uuts)
{
    nlohmann::json results;
    results["product"]      = product;
    results["overall_pass"] = std::ranges::all_of(states, [](UutState state) { return state == UutState::Passed; });
    results["uuts"]         = std::move(uuts);
    return results;
}

}    // namespace Frasy::Mcp
