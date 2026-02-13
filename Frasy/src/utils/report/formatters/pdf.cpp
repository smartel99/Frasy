/**
 * @file    table.cpp
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
#include "pdf.h"

#include <Brigerad/Core/Log.h>

#include "wkhtmltox/pdf.h"

#include <fstream>

#include <Windows.h>

namespace Frasy::Report::Formatter {
namespace Defaults {
std::string s_string = "<N/A>";
double      s_double = 0.0;
int         s_int    = 0;
}    // namespace Defaults

PDF::PDF(std::string_view outPath, std::string_view outName, const sol::table& result)
: Formatter(result), m_wkhtmltopdf(outPath, outName), m_outPath(outPath)
{
    m_ss << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head>\n"
         << "<title>" << outName << "</title>\n"
         << "<style>\n"
         << ".report-line {\n"
         << "  width: 100%;\n"
         << "  height: 4px;\n"    // thickness of the line
         << "  background: #000;\n"
         << "  margin: 6px 0;\n"
         << "}\n"
         << R"(.small-line {
    width: 100%;
    height: 4px;\n
    background: gray;
    margin: 6px 0;
}
div.info {
    width:100%;
    overflow:auto;
}
div.info div.c1 {
    width:60%;
    float:left;
}
div.info div.c2 {
    width:40%;
    float:right;
}
div.ibs {
    width:100%;
    overflow:auto;
}
div.ib {
    width:33%;
    float:left;
}
tr:nth-child(even) {
    background-color:LightGray;
})"
            "\n"
         << "</style>\n"
         << "</head>\n"
         << "<body>\n"
         << "<header style=\"text-align:center;\"><h1>Frasy Production Test Report</h1></header>\n";
    divLine();
    divLine();
}

void PDF::reportInfo()
{
    m_ss << "<div class=\"info\">\n";
    m_ss << "<div class=\"c1\">\n"
         << "<table style=\"width:100%\">";
    reportLine("Test Title", m_result["info"]["title"], Defaults::s_string);
    reportLine("Test Version", m_result["info"]["version"]["scripts"], Defaults::s_string);
    reportLine("UUT Serial Number", m_result["info"]["serial"], Defaults::s_string);
    reportLine("UUT Position", m_result["info"]["uut"], Defaults::s_int);
    m_ss << "</table></div>\n"
         << "<div class=\"c2\">\n"
         << "<table style=\"width:100%\">";
    reportLine("Test Date", m_result["info"]["date"], Defaults::s_string);
    auto old = m_ss.precision(3);
    reportLine("Test Duration (seconds)", m_result["info"]["time"]["elapsed"], Defaults::s_double);
    m_ss.precision(old);

    char  hostname[256] = {"<Unknown>"};
    DWORD size          = sizeof(hostname);
    GetUserNameA(hostname, &size);
    m_ss << "<tr>\n<td><b>Host:</b></td><td>" << hostname << "</td>\n</tr>\n";
    reportLine("Operator", m_result["info"]["operator"], Defaults::s_string);
    m_ss << "<tr>\n<td><b>Test Result:</b></td><td>";
    printResult(m_result["info"]["pass"].get_or(false));
    m_ss << "</td>\n</table>\n</div>\n</div>\n";
    divLine();
}

void PDF::reportUserInfo([[maybe_unused]] const sol::table& table)
{
    throw std::runtime_error("Not implemented");
}

void PDF::startReportIb()
{
    m_ss << "<h2>Equipment Used:</h2>\n"
         << "<div class=\"ibs\">\n"
         << "<div class=\"ib\"><h4>Frasy</h4>\n"
         << "<table style=\"width:100%\">\n";
    reportLine("Frasy Version", m_result["info"]["version"]["frasy"], Defaults::s_string);
    reportLine("Orchestrator Version", m_result["info"]["version"]["orchestrator"], Defaults::s_string);
    reportLine("Application Version", m_result["info"]["version"]["application"], Defaults::s_string);
    m_ss << "</table>\n</div>\n";
}

void PDF::reportIb(const std::string& name)
{
    const auto& ib     = m_result["ib"][name].get_or(m_emptyTable);
    std::string serial = getFieldAsStr<std::string>(ib["serial"]);
    m_ss << "<div class=\"ib\">\n"
         << "<h4>" << name << "</h4>\n"
         << "<table style=\"width:100%\">\n";
    reportLine("Serial", [&serial]() {
        std::string result;
        for (const auto& c : serial) {
            result += std::format("{:02x}", c);
        }
        return result;
    }());
    reportLine("Hardware", getFieldAsStr<std::string>(ib["hardware"]));
    reportLine("Software", getFieldAsStr<std::string>(ib["software"]));
    m_ss << "</table>\n</div>\n";
}

void PDF::endReportIb()
{
    m_ss << "</div>\n";
    divLine();
}

void PDF::startReportSequences()
{
    m_ss << "<h2>Test Sequences:</h2>\n";
}

void PDF::reportSequenceResult(const std::string& name)
{
    setSequence(name);
    auto old = m_ss.precision(3);
    m_ss << "<table style=\"width:100%;\">\n<tr>"
         << "<td style=\"width:80%;\"><h3>" << name << "</h3></td>\n"
         << "<td style=\"width:10%;\"><b>" << m_sequence["time"]["elapsed"].get_or<float>(0) << "s</b></td>\n"
         << "<td style=\"width:10%;\">" << sectionResultToString(m_sequence) << "</td>\n"
         << "</tr>\n</table>\n";
    m_ss.precision(old);

    m_ss << "<table style=\"width:100%;\">\n"
         << "<tr>\n"
         << "<th style=\"width:50%;\">Description</th>\n"
         << "<th style=\"width:13.3%;\">Tol. Min</th>\n"
         << "<th style=\"width:13.3%;\">Value</th>\n"
         << "<th style=\"width:13.3%;\">Tol. Max</th>\n"
         << "<th style=\"width:10%;\">Result</th>\n"
         << "</tr>\n";
}

void PDF::endReportSequence()
{
    m_ss << "</table>\n";
    smallDivLine();
}

void PDF::reportTestResult(const std::string& name)
{
    setTest(name);
    m_ss << "<tr>\n"
         << "<td colspan=\"4\"><b>" << name << "</b></td>\n"
         << "<td style=\"width:10%;\">" << sectionResultToString(m_test) << "</td>\n"
         << "</tr>\n";
}

void PDF::endReportTest()
{
}

bool PDF::convert()
{
    m_ss << "</body>\n"
         << "</html>\n";

    std::ofstream htmlFile(m_outPath + ".html");
    htmlFile << m_ss.str();

    m_wkhtmltopdf.addContent(m_ss.str());

    return m_wkhtmltopdf.convert();
}

void PDF::reportToBeEqualBoolean(const sol::table& expectation)
{
    reportToBeEqual<bool>(expectation);
}

void PDF::reportToBeEqualNumber(const sol::table& expectation)
{
    reportToBeEqual<double>(expectation);
}

void PDF::reportToBeEqualString(const sol::table& expectation)
{
    reportToBeEqual<std::string>(expectation);
}

void PDF::reportToBeInPercentage(const sol::table& e)
{
    reportExpectation(e, [&](const sol::table& expectation) {
        return std::format("<td>{}</td><td>{}</td><td>{}</td><td>{}</td>",
                           getFieldAsStr<double>(expectation["min"]),
                           getFieldAsStr<double>(expectation["value"]),
                           getFieldAsStr<double>(expectation["max"]),
                           sectionResultToString(expectation));
    });
}

void PDF::reportToBeInRange(const sol::table& e)
{
    reportExpectation(e, [&](const sol::table& expectation) {
        return std::format("<td>{}</td><td>{}</td><td>{}</td><td>{}</td>",
                           getFieldAsStr<double>(expectation["min"]),
                           getFieldAsStr<double>(expectation["value"]),
                           getFieldAsStr<double>(expectation["max"]),
                           sectionResultToString(expectation));
    });
}

void PDF::reportToBeGreater(const sol::table& e)
{
    reportExpectation(e, [&](const sol::table& expectation) {
        return std::format("<td>{}</td><td>{}</td><td></td><td>{}</td>",
                           getFieldAsStr<double>(expectation["min"]),
                           getFieldAsStr<double>(expectation["value"]),
                           sectionResultToString(expectation));
    });
}

void PDF::reportToBeLesser(const sol::table& e)
{
    reportExpectation(e, [&](const sol::table& expectation) {
        return std::format("<td></td><td>{}</td><td>{}</td><td>{}</td>",
                           getFieldAsStr<double>(expectation["value"]),
                           getFieldAsStr<double>(expectation["max"]),
                           sectionResultToString(expectation));
    });
}

void PDF::reportToBeNear(const sol::table& e)
{
    reportExpectation(e, [&](const sol::table& expectation) {
        return std::format("<td>{}</td><td>{}</td><td>{}</td><td>{}</td>",
                           getFieldAsStr<double>(expectation["min"]),
                           getFieldAsStr<double>(expectation["value"]),
                           getFieldAsStr<double>(expectation["max"]),
                           sectionResultToString(expectation));
    });
}

void PDF::reportSectionBaseResult([[maybe_unused]] const sol::table& section) const
{
}

std::string PDF::sectionResultToString(const sol::table& section) const
{
    if (section["skipped"] != sol::nil && section["skipped"].get<bool>()) { return "<b>SKIPPED</b>"; }
    if (section["pass"] == sol::nil) { return "<mark>Not provided</mark>"; }
    return section["pass"].get<bool>() ? "<b style=\"color:green;\">PASS</b>" : "<b style=\"color:red;\">FAIL</b>";
}

void PDF::divLine()
{
    m_ss << "<div class=\"report-line\"></div>\n";
}

void PDF::smallDivLine()
{
    m_ss << "<div class=\"small-line\"></div>\n";
}

void PDF::printResult(bool passed)
{
    if (passed) { m_ss << "<b style=\"color:green;\">PASS</b>"; }
    else {
        m_ss << "<b style=\"color:red;\">FAIL</b>";
    }
}
}    // namespace Frasy::Report::Formatter
