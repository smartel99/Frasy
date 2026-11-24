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

/**
 * v1: booleans only. A disabled scope was also flagged as skipped, so `enabled` is checked first.
 * A missing `pass` reads as a failure, like Lua does for expectation entries built by hand.
 */
Result readV1(const nlohmann::json& node, std::optional<Result> parent)
{
    if (parent == Result::Skipped || parent == Result::Disabled) { return *parent; }
    if (findBool(node, "enabled") == false) { return Result::Disabled; }
    if (findBool(node, "skipped") == true) { return Result::Skipped; }
    return findBool(node, "pass").value_or(false) ? Result::Pass : Result::Fail;
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

std::string_view toString(Result result) noexcept
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

std::optional<Result> resultFromString(std::string_view text) noexcept
{
    for (auto result : {Result::Pass, Result::Fail, Result::Skipped, Result::Disabled, Result::Incomplete}) {
        if (toString(result) == text) { return result; }
    }
    return std::nullopt;
}

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
}

Result ResultReader::read(const nlohmann::json& node, std::optional<Result> parent) const
{
    if (m_version == 1) { return readV1(node, parent); }
    return readV2(node);
}

}    // namespace Frasy::Report
