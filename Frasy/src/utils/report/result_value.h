/**
 * @file    result_value.h
 * @author  Sam Martel
 * @date    2026-11-25
 * @brief   Result of a report node and its name, without the report readers.
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

#ifndef FRASY_UTILS_REPORT_RESULT_VALUE_H
#define FRASY_UTILS_REPORT_RESULT_VALUE_H

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace Frasy::Report {

/** Outcome of a test, sequence, expectation or of the whole report (`info`). */
enum class Result : uint8_t {
    Pass,
    Fail,
    Skipped,
    Disabled,
    Incomplete,
};

/** Every result, in enumeration order. */
inline constexpr std::array s_allResults = {
  Result::Pass,
  Result::Fail,
  Result::Skipped,
  Result::Disabled,
  Result::Incomplete,
};

/**
 * @brief Name of a result as written in v2 reports ("pass", "fail", ...).
 * @return "unknown" for a value outside the enumeration.
 */
[[nodiscard]] constexpr std::string_view toString(Result result) noexcept
{
    switch (result) {
        case Result::Pass: return "pass";
        case Result::Fail: return "fail";
        case Result::Skipped: return "skipped";
        case Result::Disabled: return "disabled";
        case Result::Incomplete: return "incomplete";
    }
    return "unknown";
}

/**
 * @brief Parse a result as written in v2 reports.
 * @return The result, or nothing if @p text isn't a known result.
 */
[[nodiscard]] constexpr std::optional<Result> resultFromString(std::string_view text) noexcept
{
    for (const auto result : s_allResults) {
        if (toString(result) == text) { return result; }
    }
    return std::nullopt;
}

}    // namespace Frasy::Report

#endif    // FRASY_UTILS_REPORT_RESULT_VALUE_H
