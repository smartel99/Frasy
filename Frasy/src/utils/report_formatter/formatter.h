/**
 * @file    formatter.h
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

#ifndef FORMATTER_H
#define FORMATTER_H

#include <sol/sol.hpp>
#include <string>

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

class Formatter {
public:
                 Formatter(sol::state_view& lua, std::ofstream& output, const sol::table& result);
    virtual ~    Formatter()                                   = default;
    virtual void reportInfo()                                  = 0;
    virtual void reportVersion()                               = 0;
    virtual void reportIb(const std::string& name)             = 0;
    virtual void reportSequenceResult(const std::string& name) = 0;
    virtual void reportTestResult(const std::string& name)     = 0;
    virtual void reportToBeEqualBoolean()                      = 0;
    virtual void reportToBeEqualNumber()                       = 0;
    virtual void reportToBeEqualString()                       = 0;
    virtual void reportToBeInPercentage()                      = 0;
    virtual void reportToBeInRange()                           = 0;
    virtual void reportToBeGreater()                           = 0;
    virtual void reportToBeLesser()                            = 0;
    virtual void reportToBeNear()                              = 0;

protected:
    static std::string resultToString(const sol::object& field);
    static std::string sectionResultToString(const sol::table& section);
    void               setSequence(const std::string& name);
    void               setTest(const std::string& name);
    sol::table         getNextExpectation();
    virtual void       reportSectionBaseResult(const sol::table& section) const = 0;

    std::ofstream* m_output;
    sol::table     m_result;
    sol::table     m_emptyTable;
    std::string    m_title;
    std::string    m_sequenceName;
    sol::table     m_sequence;
    std::string    m_testName;
    sol::table     m_test;
    uint8_t        m_expectationIndex = 0;
};
}    // namespace Frasy::Report::Formatter

#endif    // FORMATTER_H
