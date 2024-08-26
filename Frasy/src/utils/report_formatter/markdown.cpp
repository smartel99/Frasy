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

#include <fstream>

namespace Frasy::Report::Formatter {

Markdown::Markdown(sol::state_view& lua, std::ofstream& output, const sol::table& result)
: Formatter(lua, output, result)
{
}

void Markdown::reportInfo()
{
    *m_output << "# Info" << endline;
    *m_output << "Date: " << m_result["info"]["date"].get<std::string>() << endline;
    *m_output << "Duration: " << m_result["info"]["time"]["elapsed"].get<double>() << endline;
    *m_output << "Operator: " << m_result["info"]["operator"].get<std::string>() << endline;
    *m_output << "Result: " << resultToString(m_result["info"]["pass"]) << endline;
}

void Markdown::reportVersion()
{
    *m_output << "# Version" << endline;
    *m_output << "Frasy: " << m_result["info"]["version"]["frasy"].get<std::string>() << endline;
    *m_output << "Orchestrator: " << m_result["info"]["version"]["orchestrator"].get<std::string>() << endline;
    *m_output << "Scripts: " << m_result["info"]["version"]["scripts"].get<std::string>() << endline;
}

void Markdown::reportIb(const std::string& name)
{
    const auto& ibs = m_result["ib"].get_or(m_emptyTable);
    const auto& ib  = ibs[name].get_or(m_emptyTable);
    *m_output << "## " << name << endline;
    *m_output << "Serial: " << getFieldAsStr<std::string>(ib["serial"]) << endline;
    *m_output << "Hardware: " << getFieldAsStr<std::string>(ib["hardware"]) << endline;
    *m_output << "Software: " << getFieldAsStr<std::string>(ib["software"]) << endline;
}

void Markdown::reportSequenceResult(const std::string& name)
{
    setSequence(name);
    *m_output << "# " << name << endline;
    reportSectionBaseResult(m_sequence);
}

void Markdown::reportTestResult(const std::string& name)
{
    setTest(name);
    *m_output << "## " << name << endline;
    reportSectionBaseResult(m_test);
}


void Markdown::reportToBeEqualBoolean()
{
    const auto expectation = getNextExpectation();
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << "Expected: " << getFieldAsStr<bool>(expectation["expected"]) << endline;
    *m_output << "Value: " << getFieldAsStr<bool>(expectation["value"]) << endline;
}

void Markdown::reportToBeEqualNumber()
{
    const auto expectation = getNextExpectation();
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline;
    *m_output << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline;
}

void Markdown::reportToBeEqualString()
{
    const auto& expectation = getNextExpectation();
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << "Expected: " << getFieldAsStr<std::string>(expectation["expected"]) << endline;
    *m_output << "Value: " << getFieldAsStr<std::string>(expectation["value"]) << endline;
}

void Markdown::reportToBeInPercentage()
{
    const auto& expectation = getNextExpectation();
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline;
    *m_output << "Percentage: " << getFieldAsStr<double>(expectation["percentage"]) << endline;
    *m_output << "Deviation: " << getFieldAsStr<double>(expectation["deviation"]) << endline;
    *m_output << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline;
}

void Markdown::reportToBeInRange()
{
    const auto& expectation = getNextExpectation();
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline;
    *m_output << "Min: " << getFieldAsStr<double>(expectation["min"]) << endline;
    *m_output << "Max: " << getFieldAsStr<double>(expectation["max"]) << endline;
    *m_output << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline;
}

void Markdown::reportToBeGreater()
{
    const auto& expectation = getNextExpectation();
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline;
    *m_output << "Min: " << getFieldAsStr<double>(expectation["min"]) << endline;
    *m_output << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline;
}

void Markdown::reportToBeLesser()
{
    const auto& expectation = getNextExpectation();
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline;
    *m_output << "Max: " << getFieldAsStr<double>(expectation["max"]) << endline;
    *m_output << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline;
}

void Markdown::reportToBeNear()
{
    const auto& expectation = getNextExpectation();
    *m_output << "### " << expectation["note"].get_or<std::string>("Unnamed") << endline;
    *m_output << "Result: " << resultToString(expectation["pass"]) << endline;
    *m_output << "Method: " << getFieldAsStr<std::string>(expectation["method"]) << endline;
    *m_output << "Inverted: " << getFieldAsStr<bool>(expectation["inverted"]) << endline;
    *m_output << "Expected: " << getFieldAsStr<double>(expectation["expected"]) << endline;
    *m_output << "Deviation: " << getFieldAsStr<double>(expectation["deviation"]) << endline;
    *m_output << "Value: " << getFieldAsStr<double>(expectation["value"]) << endline;
}

void Markdown::reportSectionBaseResult(const sol::table& section) const
{
    *m_output << "Result: " << sectionResultToString(section) << endline;
    const auto& time = section["time"].get_or(m_emptyTable);
    *m_output << "Duration: " << getFieldAsStr<double>(time["elapsed"]) << endline;
}

}    // namespace Frasy::Report::Formatter