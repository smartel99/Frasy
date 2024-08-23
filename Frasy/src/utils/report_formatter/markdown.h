/**
 * @file    markdown.h
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
 * along with this program.
 * If not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#ifndef FRASY_REPORT_FORMATTER_MARKDOWN_H
#define FRASY_REPORT_FORMATTER_MARKDOWN_H
#include <sol/sol.hpp>
namespace Frasy::Report::Formatter {
class Markdown {
public:
                             Markdown(sol::state_view& lua, std::ofstream& output);
    static std::string       resultToString(const sol::object& field);
    static std::string       sectionResultToString(const sol::table& section);
    void                     reportSectionBaseResult(const sol::table& section) const;
    void                     reportInfo(const sol::table& result) const;
    void                     reportVersion(const sol::table& result) const;
    void                     reportIb(const sol::table& result, const std::string& name) const;
    [[nodiscard]] sol::table prepareSequenceResult(const std::string_view& name, const sol::table& result) const;
    [[nodiscard]] sol::table prepareTestResult(const std::string_view& name, const sol::table& result) const;
    [[nodiscard]] sol::table getExpectation(const sol::table& test, std::size_t index) const;
    void                     reportToBeEqualBoolean(const sol::table& test, std::size_t index) const;
    void                     reportToBeEqualNumber(const sol::table& test, std::size_t index) const;
    void                     reportToBeEqualString(const sol::table& test, std::size_t index) const;
    void                     reportToBeInPercentage(const sol::table& test, std::size_t index) const;
    void                     reportToBeInRange(const sol::table& test, std::size_t index) const;
    void                     reportToBeGreater(const sol::table& test, std::size_t index) const;
    void                     reportToBeLesser(const sol::table& test, std::size_t index) const;
    void                     reportToBeNear(const sol::table& test, std::size_t index) const;
    static std::string       endline();

private:
    sol::table     m_emptyTable;
    std::ofstream* m_output;
};
};        // namespace Frasy::Report::Formatter
#endif    // FRASY_REPORT_FORMATTER_MARKDOWN_H
