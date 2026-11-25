/**
 * @file    result.cpp
 * @author  Sam Martel
 * @date    2026-11-24
 * @brief   Outcome of a report node, read from any supported report format version.
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

#include "result.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <string>
#include <utility>

namespace Frasy::Report {

namespace {

/** Value of a boolean field, nothing if absent. */
std::optional<bool> findBool(const nlohmann::json& node, std::string_view key)
{
    const auto it = node.find(key);
    if (it == node.end()) { return std::nullopt; }
    if (!it->is_boolean()) { throw ReportFormatError(std::format("'{}' is not a boolean: {}", key, it->dump())); }
    return it->get<bool>();
}

bool isScopeV1(const nlohmann::json& node)
{
    return node.is_object() && (node.contains("pass") || node.contains("skipped") || node.contains("enabled"));
}

/**
 * v1 booleans of a node. A disabled scope was also flagged as skipped, so `enabled` is checked first.
 * A missing `pass` reads as a failure, like Lua does for expectation entries built by hand.
 */
Result readV1Booleans(const nlohmann::json& node)
{
    if (findBool(node, "enabled") == false) { return Result::Disabled; }
    if (findBool(node, "skipped") == true) { return Result::Skipped; }
    return findBool(node, "pass").value_or(false) ? Result::Pass : Result::Fail;
}

/** Test that never ran, e.g. in a section cut short by an error. The orchestrator leaves it out of its sequence. */
bool isPendingV1(const nlohmann::json& test)
{
    const auto time = test.find("time");
    if (time == test.end() || !time->is_object()) { return false; }
    const auto stop = time->find("stop");
    return stop != time->end() && stop->is_number() && stop->get<double>() == 0.0;
}

/** v1 sequence that isn't a pass: incomplete if it holds a disabled test and no failed one. */
bool isIncompleteV1(const nlohmann::json& node)
{
    const auto tests = node.find("tests");
    if (tests == node.end() || !tests->is_object()) { return false; }
    bool disabled = false;
    for (const auto& test : *tests) {
        if (!isScopeV1(test) || isPendingV1(test)) { continue; }
        const auto result = readV1Booleans(test);
        if (result == Result::Fail) { return false; }
        disabled = disabled || result == Result::Disabled;
    }
    return disabled;
}

/** v1: booleans only, plus the incomplete sequence rule. */
Result readV1(const nlohmann::json& node, std::optional<Result> parent)
{
    if (parent == Result::Skipped || parent == Result::Disabled) { return *parent; }
    const auto result = readV1Booleans(node);
    if (result != Result::Fail) { return result; }
    return isIncompleteV1(node) ? Result::Incomplete : Result::Fail;
}

/** v1 board: incomplete as soon as a sequence or test is disabled, like the orchestrator decides it. */
bool hasDisabledScopeV1(const nlohmann::json& report)
{
    const auto sequences = report.find("sequences");
    if (sequences == report.end() || !sequences->is_object()) { return false; }
    for (const auto& sequence : *sequences) {
        if (!isScopeV1(sequence)) { continue; }
        if (findBool(sequence, "enabled") == false) { return true; }
        const auto tests = sequence.find("tests");
        if (tests == sequence.end() || !tests->is_object()) { continue; }
        for (const auto& test : *tests) {
            if (isScopeV1(test) && findBool(test, "enabled") == false) { return true; }
        }
    }
    return false;
}

Result readV2(const nlohmann::json& node)
{
    const auto it = node.find("result");
    if (it == node.end() || !it->is_string()) { throw ReportFormatError("v2 report node has no string 'result'"); }
    const auto& text   = it->get_ref<const std::string&>();
    const auto  result = resultFromString(text);
    if (!result) { throw ReportFormatError(std::format("Unknown result '{}'", text)); }
    return *result;
}

/** Whole number held by @p value, nothing if it has a fraction or doesn't fit. */
std::optional<int64_t> toWholeNumber(const nlohmann::json& value)
{
    if (value.is_number_unsigned()) {
        const auto number = value.get<uint64_t>();
        if (!std::in_range<int64_t>(number)) { return std::nullopt; }
        return static_cast<int64_t>(number);
    }
    if (value.is_number_integer()) { return value.get<int64_t>(); }
    if (value.is_number_float()) {
        // SaveAsJson writes every Lua number as a double, so the version is saved as `2.0`.
        const auto number = value.get<double>();
        if (!std::isfinite(number) || std::trunc(number) != number) { return std::nullopt; }
        if (number < static_cast<double>(std::numeric_limits<int>::min()) ||
            number > static_cast<double>(std::numeric_limits<int>::max())) {
            return std::nullopt;
        }
        return static_cast<int64_t>(number);
    }
    return std::nullopt;
}

}    // namespace

int getFormatVersion(const nlohmann::json& report)
{
    const auto info = report.find("info");
    if (info == report.end() || !info->is_object()) { throw ReportFormatError("Report has no 'info'"); }
    const auto version = info->find("format_version");
    if (version == info->end()) { return 1; }
    const auto number = toWholeNumber(*version);
    if (!number || *number < 1 || !std::in_range<int>(*number)) {
        throw ReportFormatError(std::format("Invalid format_version {}", version->dump()));
    }
    return static_cast<int>(*number);
}

ResultReader::ResultReader(const nlohmann::json& report) : m_version(Report::getFormatVersion(report))
{
    if (m_version > s_currentFormatVersion) {
        throw ReportFormatError(
          std::format("Report format_version {} is newer than supported ({})", m_version, s_currentFormatVersion));
    }
    m_info = read(report.at("info"));
    if (m_version == 1 && hasDisabledScopeV1(report)) { m_info = Result::Incomplete; }
}

Result ResultReader::read(const nlohmann::json& node, std::optional<Result> parent) const
{
    if (m_version == 1) { return readV1(node, parent); }
    return readV2(node);
}

bool ResultReader::isScope(const nlohmann::json& node) const noexcept
{
    if (!node.is_object()) { return false; }
    if (m_version == 1) { return isScopeV1(node); }
    return node.contains("result");
}

namespace {

/** v1 booleans of an in-process node, nothing if it carries none. */
std::optional<Result> readBooleans(const sol::table& node)
{
    if (const sol::optional<bool> enabled = node["enabled"]; enabled && !*enabled) { return Result::Disabled; }
    if (const sol::optional<bool> skipped = node["skipped"]; skipped && *skipped) { return Result::Skipped; }
    const sol::optional<bool> pass = node["pass"];
    if (!pass) { return std::nullopt; }
    return *pass ? Result::Pass : Result::Fail;
}

/** Test that never ran: the orchestrator leaves it out of its sequence. */
bool isPending(const sol::table& test)
{
    const sol::optional<sol::table> time = test["time"];
    if (!time) { return false; }
    const sol::optional<double> stop = (*time)["stop"];
    return stop && *stop == 0.0;
}

}    // namespace

std::optional<Result> getResult(const sol::table& node)
{
    if (!node.valid()) { return std::nullopt; }
    if (const sol::optional<std::string> text = node["result"]; text) {
        if (const auto result = resultFromString(*text)) { return result; }
    }
    const auto result = readBooleans(node);
    if (result != Result::Fail) { return result; }
    // v1 sequence: incomplete if it holds a disabled test and no failed one.
    const sol::optional<sol::table> tests = node["tests"];
    if (!tests) { return Result::Fail; }
    bool disabled = false;
    for (const auto& [key, value] : *tests) {
        if (!value.is<sol::table>()) { continue; }
        const auto test = value.as<sol::table>();
        if (isPending(test)) { continue; }
        const auto testResult = readBooleans(test);
        if (testResult == Result::Fail) { return Result::Fail; }
        disabled = disabled || testResult == Result::Disabled;
    }
    return disabled ? Result::Incomplete : Result::Fail;
}

std::optional<Result> getBoardResult(const sol::table& report)
{
    if (!report.valid()) { return std::nullopt; }
    const sol::optional<sol::table> info = report["info"];
    if (!info) { return std::nullopt; }
    if (info->get<sol::optional<int>>("format_version")) { return getResult(*info); }
    // v1: a stray `info.result` is ignored. Incomplete as soon as a sequence or test is disabled.
    const auto result = readBooleans(*info);
    if (!result) { return result; }
    const sol::optional<sol::table> sequences = report["sequences"];
    if (!sequences) { return result; }
    for (const auto& [sKey, sValue] : *sequences) {
        if (!sValue.is<sol::table>()) { continue; }
        const auto sequence = sValue.as<sol::table>();
        if (sequence.get<sol::optional<bool>>("enabled") == false) { return Result::Incomplete; }
        const sol::optional<sol::table> tests = sequence["tests"];
        if (!tests) { continue; }
        for (const auto& [tKey, tValue] : *tests) {
            if (tValue.is<sol::table>() && tValue.as<sol::table>().get<sol::optional<bool>>("enabled") == false) {
                return Result::Incomplete;
            }
        }
    }
    return result;
}

nlohmann::json ResultCounts::toJson() const
{
    nlohmann::json j = nlohmann::json::object();
    for (const auto result : s_allResults) {
        j[std::string(toString(result))] = get(result);
    }
    return j;
}

ResultCounts countTests(const nlohmann::json& report)
{
    const ResultReader reader(report);
    ResultCounts       counts;
    const auto         sequences = report.find("sequences");
    if (sequences == report.end() || !sequences->is_object()) { return counts; }
    for (const auto& sequence : *sequences) {
        if (!reader.isScope(sequence)) { continue; }
        const auto result = reader.read(sequence);
        const auto tests  = sequence.find("tests");
        if (tests == sequence.end() || !tests->is_object()) { continue; }
        for (const auto& test : *tests) {
            if (reader.isScope(test)) { counts.add(reader.read(test, result)); }
        }
    }
    return counts;
}

bool hasFailedScope(const nlohmann::json& report)
{
    const ResultReader reader(report);
    const auto         sequences = report.find("sequences");
    if (sequences == report.end() || !sequences->is_object()) { return false; }
    for (const auto& sequence : *sequences) {
        if (!reader.isScope(sequence)) { continue; }
        const auto result = reader.read(sequence);
        if (result == Result::Fail) { return true; }
        const auto tests = sequence.find("tests");
        if (tests == sequence.end() || !tests->is_object()) { continue; }
        for (const auto& test : *tests) {
            if (reader.isScope(test) && reader.read(test, result) == Result::Fail) { return true; }
        }
    }
    return false;
}

}    // namespace Frasy::Report
