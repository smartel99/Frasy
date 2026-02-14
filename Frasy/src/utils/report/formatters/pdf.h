/**
 * @file    table.h
 * @author  Sam Martel
 * @date    2026-02-02
 * @brief
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


#ifndef FRASY_SRC_UTILS_REPORT_FORMATTERS_TABLE_H
#define FRASY_SRC_UTILS_REPORT_FORMATTERS_TABLE_H

#include "formatter.h"
#include <sol/sol.hpp>

#include "../utils/wkhtmltopdf.h"

#include <sstream>
#include <string_view>

namespace Frasy::Report::Formatter {

class PDF final : public Formatter {
public:
    PDF(std::string_view outPath, std::string_view outName, const sol::table& result);
    ~PDF() override = default;
    void reportInfo() override;
    void reportUserInfo(const sol::table& table) override;
    void startReportIb();
    void reportIb(const std::string& name) override;
    void endReportIb();

    void startReportSequences();
    void reportSequenceResult(const std::string& name) override;
    void endReportSequence();

public:
    void reportTestResult(const std::string& name) override;
    void endReportTest();

    bool convert();

protected:
    void reportExpectation(const sol::table& expectation, auto&& F)
    {
        auto name = expectation["name"].get_or<std::string>("Not Provided");
        auto note = expectation["note"].get_or<std::string>("Not Provided");
        m_ss << "<tr>\n"
             << "<td>&nbsp&nbsp" << name;
        if (name != note) { m_ss << " (" << note << ")"; }
        m_ss << "</td>\n" << F(expectation) << "</tr>\n";
    }

    template<typename T>
    void reportToBeEqual(const sol::table& e)
    {
        reportExpectation(e, [&](const sol::table& expectation) {
            return std::format("<td></td><td>{}</td><td></td><td>{}</td>",
                               getFieldAsStr<T>(expectation["value"]),
                               sectionResultToString(expectation));
        });
    }

    void        reportToBeEqualBoolean(const sol::table& expectation) override;
    void        reportToBeEqualNumber(const sol::table& expectation) override;
    void        reportToBeEqualString(const sol::table& expectation) override;
    void        reportToBeInPercentage(const sol::table& e) override;
    void        reportToBeInRange(const sol::table& e) override;
    void        reportToBeGreater(const sol::table& e) override;
    void        reportToBeLesser(const sol::table& e) override;
    void        reportToBeNear(const sol::table& e) override;
    void        reportSectionBaseResult(const sol::table& section) const override;
    std::string sectionResultToString(const sol::table& section) const override;

private:
    void divLine();
    void smallDivLine();
    void printResult(bool passed);

    void reportLine(std::string_view fieldName, auto field)
    {
        m_ss << "<tr>\n"
             << "<td><b>" << fieldName << "</b>:</td> "
             << "<td>" << field << "</td>\n"
             << "</tr>\n";
    }

    void reportLine(std::string_view fieldName, auto field, auto defaultVal)
    {
        reportLine(fieldName, (field.valid() ? field.template get_or<decltype(defaultVal)>(defaultVal) : defaultVal));
    }

private:
    Details::Wkhtmltopdf m_wkhtmltopdf;
    std::stringstream    m_ss;
    std::string          m_outPath;
};
}    // namespace Frasy::Report::Formatter
#endif    // FRASY_SRC_UTILS_REPORT_FORMATTERS_TABLE_H
