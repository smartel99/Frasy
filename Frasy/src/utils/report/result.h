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

#include "result_value.h"
#include "utils/UutState.h"

#include <json.hpp>
#include <sol/sol.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace Frasy::Report {

/** How many times a scope ended with each result. */
struct ResultCounts {
    std::array<size_t, s_allResults.size()> Counts = {};    //!< Indexed by Result.

    void add(Result result) { Counts.at(static_cast<size_t>(result))++; }

    [[nodiscard]] size_t get(Result result) const { return Counts.at(static_cast<size_t>(result)); }

    [[nodiscard]] size_t getTotal() const
    {
        size_t total = 0;
        for (const auto count : Counts) {
            total += count;
        }
        return total;
    }

    /** Share of all runs that ended with @p result, in percent. 0 without any run. */
    [[nodiscard]] float getPercent(Result result) const
    {
        const auto total = getTotal();
        return total == 0 ? 0.0f : static_cast<float>(get(result)) * 100.0f / static_cast<float>(total);
    }

    /**
     * @brief Pass rate among the runs that got a verdict, in percent.
     * Skipped, disabled and incomplete runs are left out: they say nothing about the board's quality.
     * @return pass / (pass + fail), 0 without any verdict.
     */
    [[nodiscard]] float getPassRate() const
    {
        const auto verdicts = get(Result::Pass) + get(Result::Fail);
        return verdicts == 0 ? 0.0f : static_cast<float>(get(Result::Pass)) * 100.0f / static_cast<float>(verdicts);
    }

    /** @return The counts as an object keyed by result name ("pass", "fail", ...). */
    [[nodiscard]] nlohmann::json toJson() const;
};

/** Report layout written by the current orchestrator (`info.format_version`). */
inline constexpr int s_currentFormatVersion = 2;

/** A report that can't be read: unsupported version, or a result of the wrong type or unknown value. */
class ReportFormatError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

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

/**
 * @brief How many tests of a report ended with each result.
 * @param report The whole report.
 * @throws ReportFormatError if the report or a scope result can't be read.
 */
[[nodiscard]] ResultCounts countTests(const nlohmann::json& report);

/**
 * @brief Result of a UUT: its report's `info.result`, or Disabled for a UUT that was turned off and never ran.
 * @return The result, or nothing for states that aren't one (idle, waiting, running, error).
 */
[[nodiscard]] constexpr std::optional<Result> getUutResult(UutState state) noexcept
{
    switch (state) {
        case UutState::Passed: return Result::Pass;
        case UutState::Failed: return Result::Fail;
        case UutState::Disabled: return Result::Disabled;
        case UutState::IncompletePassed:
        case UutState::IncompleteFailed: return Result::Incomplete;
        case UutState::Idle:
        case UutState::Waiting:
        case UutState::Running:
        case UutState::Error: break;
    }
    return std::nullopt;
}

}    // namespace Frasy::Report

#endif    // FRASY_UTILS_REPORT_RESULT_H
