/**
 * @file    result.h
 * @author  Sam Martel
 * @date    2026-11-24
 * @brief   Outcome of a report node, read from any supported report format version.
 *
 * Saved reports (JSON files) can be of any version and go through ResultReader. Report tables inside
 * the running process (`onReport`, the report formatters) are always s_currentFormatVersion: read their
 * `result` field with resultFromString().
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

#ifndef FRASY_UTILS_REPORT_RESULT_H
#define FRASY_UTILS_REPORT_RESULT_H

#include <json.hpp>
#include <sol/sol.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
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

/** Report layout written by the current orchestrator (`info.format_version`). */
inline constexpr int s_currentFormatVersion = 2;

/** A report that can't be read: unsupported version, or a result of the wrong type or unknown value. */
class ReportFormatError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * @brief Name of a result as written in v2 reports ("pass", "fail", ...).
 * @return "unknown" for a value outside the enumeration.
 */
[[nodiscard]] std::string_view toString(Result result) noexcept;

/**
 * @brief Parse a result as written in v2 reports.
 * @return The result, or nothing if @p text isn't a known result.
 */
[[nodiscard]] std::optional<Result> resultFromString(std::string_view text) noexcept;

/**
 * @brief Layout version of a report.
 * @param report The whole report.
 * @return `info.format_version`, or 1 for reports written before it existed. Whole floats (`2.0`, as written by
 *         SaveAsJson) are accepted.
 * @throws ReportFormatError if `info` is missing or the version isn't a positive whole number fitting an `int`.
 */
[[nodiscard]] int getFormatVersion(const nlohmann::json& report);

/**
 * Reads the outcomes of a saved report, whatever its layout version.
 *
 * The version is read once, when the reader is built, and selects how every node is read.
 */
class ResultReader {
public:
    /**
     * @param report The whole report. The reader doesn't keep a reference to it.
     * @throws ReportFormatError if the version is invalid or newer than s_currentFormatVersion, or `info` has no
     *         readable result.
     */
    explicit ResultReader(const nlohmann::json& report);

    [[nodiscard]] int getFormatVersion() const noexcept { return m_version; }

    /** Outcome of the whole report (`info`). */
    [[nodiscard]] Result getInfo() const noexcept { return m_info; }

    /**
     * @brief Outcome of a sequence, test or expectation.
     * @param node   The node to read.
     * @param parent Result of the enclosing scope. v1 reports left the tests of a skipped or disabled sequence
     *               untouched, so they take the sequence's result. v2 reports already carry it.
     * @return The node's result.
     * @throws ReportFormatError if the node's result is of the wrong type or unknown.
     */
    [[nodiscard]] Result read(const nlohmann::json& node, std::optional<Result> parent = std::nullopt) const;

    /**
     * @brief Whether @p node carries a result in this reader's layout.
     * Entries without one aren't scopes, e.g. added by a product's `onReport`.
     * @return true if @p node is a sequence, test or expectation result.
     */
    [[nodiscard]] bool isScope(const nlohmann::json& node) const noexcept;

private:
    int    m_version = 1;
    Result m_info    = Result::Fail;
};

/**
 * @brief Result of a node of an in-process report table (`info`, sequence, test or expectation).
 * In-process tables are always s_currentFormatVersion: `result` is read. Entries a product built by hand may only carry
 * the deprecated booleans; they are read like v1 nodes (`enabled`, then `skipped`, then `pass`).
 * @return The result, or nothing if the node carries none of them.
 */
[[nodiscard]] std::optional<Result> getResult(const sol::table& node);

/**
 * @brief Whether any sequence or test of @p report failed.
 * Tells an incomplete board where something that ran failed from one where everything that ran passed. Tests are read
 * as well as sequences: a sequence cut short by an error in a later section keeps the result of its earlier sections.
 * @param report The whole report.
 * @return true if a sequence or test result is a failure.
 * @throws ReportFormatError if the report or a scope result can't be read.
 */
[[nodiscard]] bool hasFailedScope(const nlohmann::json& report);

}    // namespace Frasy::Report

#endif    // FRASY_UTILS_REPORT_RESULT_H
