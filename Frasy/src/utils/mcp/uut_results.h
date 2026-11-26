/**
 * @file    uut_results.h
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

#ifndef FRASY_UTILS_MCP_UUT_RESULTS_H
#define FRASY_UTILS_MCP_UUT_RESULTS_H

#include "utils/UutState.h"

#include <json.hpp>

#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace Frasy::Mcp {

/**
 * @brief Results of one UUT.
 * @param reportPath The UUT's last report. Without a readable one, the test counts are zero and the error is logged.
 * @return `uut`, `serial`, `pass`, `state`, `result` (missing for a UUT that ended in error), `tests_passed`,
 * `tests_total`, `test_results`, and `report_path` and `duration` when the report exists.
 */
[[nodiscard]] nlohmann::json makeUutResult(std::size_t                  uut,
                                           std::string_view             serial,
                                           UutState                     state,
                                           const std::filesystem::path& reportPath);

/**
 * @brief Results of a run.
 * @param uuts The results of each UUT, from makeUutResult().
 * @param states The state of each UUT.
 * @return `product`, `overall_pass` (every UUT passed) and `uuts`.
 */
[[nodiscard]] nlohmann::json makeRunResults(std::string_view          product,
                                            std::span<const UutState> states,
                                            nlohmann::json            uuts);

}    // namespace Frasy::Mcp

#endif    // FRASY_UTILS_MCP_UUT_RESULTS_H
