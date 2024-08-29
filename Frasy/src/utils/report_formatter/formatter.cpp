/**
 * @file    formatter.cpp
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
#include "formatter.h"

namespace Frasy::Report::Formatter {

Formatter::Formatter(sol::state_view& lua, std::ofstream& output, const sol::table& result)
: m_emptyTable(lua.create_table()), m_output(&output), m_result(result)
{
}

void Formatter::setSequence(const std::string& name)
{
    const auto& sequences = m_result["sequences"].get_or(m_emptyTable);
    m_sequence            = sequences[name].get_or(m_emptyTable);
    m_sequenceName        = name;
    m_expectationIndex    = 0;
}

void Formatter::setTest(const std::string& name)
{
    const auto& tests  = m_sequence["tests"].get_or(m_emptyTable);
    m_test             = tests[name].get_or(m_emptyTable);
    m_testName         = name;
    m_expectationIndex = 0;
}



sol::table Formatter::getNextExpectation()
{
    const auto& expectations = m_test["expectations"].get_or(m_emptyTable);
    auto        expectation  = expectations[m_expectationIndex + 1].get_or(m_emptyTable);
    m_expectationIndex++;
    return expectation;
}

std::string Formatter::resultToString(const sol::object& field)
{
    if (field == sol::nil) { return "Not provided"; }
    return (field.as<bool>() ? "PASS" : "FAIL");
}

std::string Formatter::sectionResultToString(const sol::table& section)
{
    if (section["skipped"] != sol::nil && section["skipped"].get<bool>()) { return "SKIPPED"; }
    if (section["pass"] == sol::nil) { return "Not provided"; }
    return section["pass"].get<bool>() ? "PASS" : "FAIL";
}

}    // namespace Frasy::Report::Formatter