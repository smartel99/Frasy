/**
 * @file    keyValue.cpp
 * @author  Paul Thomas
 * @date    8/26/2024
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version. This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details. You should have received a copy of the GNU General Public License
 * along with this program. If not, see <a
 * href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#include "keyValue.h"

#include "spdlog/fmt/bundled/format.h"
#include "spdlog/fmt/bundled/ranges.h"

#include <fstream>

namespace Frasy::Report::Formatter {


KeyValue::KeyValue(sol::state_view& lua, std::ofstream& output, const sol::table& result)
: Formatter(lua, output, result)
{
}

void KeyValue::reportInfo()
{
    *m_output << "Info-Date: " << m_result["info"]["date"].get_or<std::string>("<N/A>") << endline;
    *m_output << "Info-Duration: " << m_result["info"]["time"]["elapsed"].get_or<double>(0.0f) << endline;
    *m_output << "Info-Operator: " << m_result["info"]["operator"].get_or<std::string>("<N/A>") << endline;
    *m_output << "Info-Result: " << resultToString(m_result["info"]["pass"]) << endline;
}

void KeyValue::reportVersion()
{
    *m_output << "Version-Frasy: " << m_result["info"]["version"]["frasy"].get<std::string>() << endline;
    *m_output << "Version-Orchestrator: " << m_result["info"]["version"]["orchestrator"].get<std::string>() << endline;
    *m_output << "Version-Scripts: " << m_result["info"]["version"]["scripts"].get<std::string>() << endline;
}

void KeyValue::reportIb(const std::string& name)
{
    const auto& ibs    = m_result["ib"].get_or(m_emptyTable);
    const auto& ib     = ibs[name].get_or(m_emptyTable);
    const auto  prefix = "Version-" + name + "-";

    std::string serial = getFieldAsStr<std::string>(ib["serial"]);
    *m_output << prefix << "Serial: " << fmt::format("{:02x}", fmt::join(serial, "")) << endline;
    *m_output << prefix << "Hardware: " << getFieldAsStr<std::string>(ib["hardware"]) << endline;
    *m_output << prefix << "Software: " << getFieldAsStr<std::string>(ib["software"]) << endline;
}

void KeyValue::reportSequenceResult(const std::string& name)
{
    setSequence(name);
    m_sectionPrefix = name;
    reportSectionBaseResult(m_sequence);
}

void KeyValue::reportTestResult(const std::string& name)
{
    setTest(name);
    m_sectionPrefix = m_sequenceName + "-" + name;
    reportSectionBaseResult(m_test);
}

void KeyValue::reportToBeEqualBoolean(const sol::table& expectation)
{
    const auto prefix      = m_sectionPrefix + "-" + std::to_string(m_expectationIndex) + "-";
    *m_output << prefix << "Note: " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << prefix << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << prefix << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << prefix << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << prefix << "Expected: " << getFieldAsStr<bool>(expectation["expected"]) << endline;
    *m_output << prefix << "Value: " << getFieldAsStr<bool>(expectation["value"]) << endline;
}

void KeyValue::reportToBeEqualNumber(const sol::table& expectation)
{
    const auto prefix      = m_sectionPrefix + "-" + std::to_string(m_expectationIndex) + "-";
    *m_output << prefix << "Note: " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << prefix << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << prefix << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << prefix << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << prefix << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline;
    *m_output << prefix << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline;
}

void KeyValue::reportToBeEqualString(const sol::table& expectation)
{
    const auto  prefix      = m_sectionPrefix + "-" + std::to_string(m_expectationIndex) + "-";
    *m_output << prefix << "Note: " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << prefix << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << prefix << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << prefix << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << prefix << "Expected: " << getFieldAsStr<std::string>(expectation["expected"]) << endline;
    *m_output << prefix << "Value: " << getFieldAsStr<std::string>(expectation["value"]) << endline;
}

void KeyValue::reportToBeInPercentage(const sol::table& expectation)
{
    const auto  prefix      = m_sectionPrefix + "-" + std::to_string(m_expectationIndex) + "-";
    *m_output << prefix << "Note: " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << prefix << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << prefix << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << prefix << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << prefix << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline;
    *m_output << prefix << "Percentage: " << getFieldAsStr<double>(expectation["percentage"]) << endline;
    *m_output << prefix << "Deviation: " << getFieldAsStr<double>(expectation["deviation"]) << endline;
    *m_output << prefix << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline;
}

void KeyValue::reportToBeInRange(const sol::table& expectation)
{
    const auto  prefix      = m_sectionPrefix + "-" + std::to_string(m_expectationIndex) + "-";
    *m_output << prefix << "Note: " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << prefix << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << prefix << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << prefix << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << prefix << "Min: " << getFieldAsStr<double>(expectation["min"]) << endline;
    *m_output << prefix << "Max: " << getFieldAsStr<double>(expectation["max"]) << endline;
    *m_output << prefix << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline;
}

void KeyValue::reportToBeGreater(const sol::table& expectation)
{
    const auto  prefix      = m_sectionPrefix + "-" + std::to_string(m_expectationIndex) + "-";
    *m_output << prefix << "Note: " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << prefix << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << prefix << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << prefix << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << prefix << "Min: " << getFieldAsStr<double>(expectation["min"]) << endline;
    *m_output << prefix << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline;
}

void KeyValue::reportToBeLesser(const sol::table& expectation)
{
    const auto  prefix      = m_sectionPrefix + "-" + std::to_string(m_expectationIndex) + "-";
    *m_output << prefix << "Note: " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << prefix << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << prefix << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << prefix << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << prefix << "Max: " << getFieldAsStr<double>(expectation["max"]) << endline;
    *m_output << prefix << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline;
}

void KeyValue::reportToBeNear(const sol::table& expectation)
{
    const auto  prefix      = m_sectionPrefix + "-" + std::to_string(m_expectationIndex) + "-";
    *m_output << prefix << "Note: " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << prefix << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << prefix << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << prefix << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << prefix << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline;
    *m_output << prefix << "Deviation: " << getFieldAsStr<double>(expectation["deviation"]) << endline;
    *m_output << prefix << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline;
}

void KeyValue::reportSectionBaseResult(const sol::table& section) const
{
    *m_output << m_sectionPrefix << "-Result: " << sectionResultToString(section) << endline;
    const auto& time = section["time"].get_or(m_emptyTable);
    *m_output << m_sectionPrefix << "-Duration: " << getFieldAsStr<double>(time["elapsed"]) << endline;
}

}    // namespace Frasy::Report::Formatter
