/**
 * @file    markdown.cpp
 * @author  Paul Thomas
 * @date    8/22/2024
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

#include "markdown.h"

#include "Brigerad/Core/Log.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "../../frasy/version.h"

namespace Frasy::Report::Formatter {
template<typename T>
    requires !std::is_same_v<T, std::string>
             static std::string getFieldAsStr(const sol::object& field)
{
    if (field == sol::nil) { return "Not provided"; }
    return std::to_string(field.as<T>());
}

template<typename T>
    requires std::is_same_v<T, std::string>
static std::string getFieldAsStr(const sol::object& field)
{
    if (field == sol::nil) { return "Not provided"; }
    return field.as<T>();
}

Markdown::Markdown(sol::state_view& lua, std::ofstream& output) : m_emptyTable(lua.create_table()), m_output(&output)
{
}


std::string Markdown::resultToString(const sol::object& field)
{
    if (field == sol::nil) { return "Not provided"; }
    return (field.as<bool>() ? "PASS" : "FAIL");
}

std::string Markdown::sectionResultToString(const sol::table& section)
{
    if (section["skipped"] != sol::nil && section["skipped"].get<bool>()) { return "SKIPPED"; }
    if (section["pass"] == sol::nil) { return "Not provided"; }
    return section["pass"].get<bool>() ? "PASS" : "FAIL";
}

void Markdown::reportSectionBaseResult(const sol::table& section) const
{
    *m_output << "Result: " << sectionResultToString(section) << endline();
    const auto& time = section["time"].get_or(m_emptyTable);
    *m_output << "Duration: " << getFieldAsStr<double>(time["elapsed"]) << endline();
}

void Markdown::reportInfo(const sol::table& result) const
{
    *m_output << "# Info" << endline();
    *m_output << "Date: " << result["info"]["date"].get<std::string>() << endline();
    *m_output << "Duration: " << result["info"]["time"]["elapsed"].get<double>() << endline();
    *m_output << "Operator: " << result["info"]["operator"].get<std::string>() << endline();
    *m_output << "Result: " << resultToString(result["info"]["pass"]) << endline();
}

void Markdown::reportVersion(const sol::table& result) const
{
    *m_output << "# Version" << endline();
    *m_output << "Frasy: " << result["info"]["version"]["frasy"].get<std::string>() << endline();
    *m_output << "Orchestrator: " << result["info"]["version"]["orchestrator"].get<std::string>() << endline();
    *m_output << "Scripts: " << result["info"]["version"]["scripts"].get<std::string>() << endline();
}

void Markdown::reportIb(const sol::table& result, const std::string& name) const
{
    const auto& ibs = result["ib"].get_or(m_emptyTable);
    const auto& ib  = ibs[name].get_or(m_emptyTable);
    *m_output << "## " << name << endline();
    *m_output << "Serial: " << getFieldAsStr<std::string>(ib["serial"]) << endline();
    *m_output << "Hardware: " << getFieldAsStr<std::string>(ib["hardware"]) << endline();
    *m_output << "Software: " << getFieldAsStr<std::string>(ib["software"]) << endline();
}

sol::table Markdown::prepareSequenceResult(const std::string_view& name, const sol::table& result) const
{
    const auto& sequences = result["sequences"].get_or(m_emptyTable);
    auto        sequence  = sequences[name].get_or(m_emptyTable);
    *m_output << "# " << name << endline();
    reportSectionBaseResult(sequence);
    return sequence;
}

sol::table Markdown::prepareTestResult(const std::string_view& name, const sol::table& sequence) const
{
    const auto& tests = sequence["tests"].get_or(m_emptyTable);
    sol::table  test  = tests[name].get_or(m_emptyTable);
    *m_output << "## " << name << endline();
    reportSectionBaseResult(test);
    return test.as<sol::table>();
}

sol::table Markdown::getExpectation(const sol::table& test, std::size_t index) const
{
    const auto& expectations = test["expectations"].get_or(m_emptyTable);
    return expectations[index].get_or(m_emptyTable);
}

void Markdown::reportToBeEqualBoolean(const sol::table& test, std::size_t index) const
{
    sol::table expectation = getExpectation(test, index);
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline();
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline();
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline();
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline();
    *m_output << "Expected: " << getFieldAsStr<bool>(expectation["expected"]) << endline();
    *m_output << "Value: " << getFieldAsStr<bool>(expectation["value"]) << endline();
}

void Markdown::reportToBeEqualNumber(const sol::table& test, std::size_t index) const
{
    auto expectation = getExpectation(test, index);
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline();
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline();
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline();
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline();
    *m_output << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline();
    *m_output << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline();
}

void Markdown::reportToBeEqualString(const sol::table& test, std::size_t index) const
{
    auto expectation = getExpectation(test, index);
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline();
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline();
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline();
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline();
    *m_output << "Expected: " << getFieldAsStr<std::string>(expectation["expected"]) << endline();
    *m_output << "Value: " << getFieldAsStr<std::string>(expectation["value"]) << endline();
}

void Markdown::reportToBeInPercentage(const sol::table& test, std::size_t index) const
{
    auto expectation = getExpectation(test, index);
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline();
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline();
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline();
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline();
    *m_output << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline();
    *m_output << "Percentage: " << getFieldAsStr<double>(expectation["percentage"]) << endline();
    *m_output << "Deviation: " << getFieldAsStr<double>(expectation["deviation"]) << endline();
    *m_output << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline();
}

void Markdown::reportToBeInRange(const sol::table& test, std::size_t index) const
{
    auto expectation = getExpectation(test, index);
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline();
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline();
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline();
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline();
    *m_output << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline();
    *m_output << "Min: " << getFieldAsStr<double>(expectation["min"]) << endline();
    *m_output << "Max: " << getFieldAsStr<double>(expectation["max"]) << endline();
    *m_output << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline();
}

void Markdown::reportToBeGreater(const sol::table& test, std::size_t index) const
{
    auto expectation = getExpectation(test, index);
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline();
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline();
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline();
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline();
    *m_output << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline();
    *m_output << "Min: " << getFieldAsStr<double>(expectation["min"]) << endline();
    *m_output << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline();
}

void Markdown::reportToBeLesser(const sol::table& test, std::size_t index) const
{
    auto expectation = getExpectation(test, index);
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline();
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline();
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline();
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline();
    *m_output << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline();
    *m_output << "Max: " << getFieldAsStr<double>(expectation["max"]) << endline();
    *m_output << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline();
}

void Markdown::reportToBeNear(const sol::table& test, std::size_t index) const
{
    auto expectation = getExpectation(test, index);
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline();
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline();
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline();
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline();
    *m_output << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline();
    *m_output << "Deviation: " << getFieldAsStr<double>(expectation["deviation"]) << endline();
    *m_output << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline();
}

std::string Markdown::endline()
{
    return "  \n";
}

}    // namespace Frasy::Report::Formatter