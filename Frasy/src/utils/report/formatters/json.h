/**
 * @file    json.h
 * @author  Paul Thomas
 * @date    3/17/2025
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

#ifndef FRASY_SRC_UTILS_REPORT_FORMATTERS_JSON_H
#define FRASY_SRC_UTILS_REPORT_FORMATTERS_JSON_H

#include "formatter.h"
#include <json.hpp>

namespace Frasy::Report::Formatter {
class Json final : public Formatter {
public:
    Json(const sol::table& result);
    void reportInfo() override;
    void reportUserInfo(const sol::table& table) override;
    void reportIb(const std::string& name) override;
    void reportSequenceResult(const std::string& name) override;
    void reportTestResult(const std::string& name) override;
    void toFile(const std::string& filename);

protected:
    void reportToBeEqualBoolean(const sol::table& expectation) override { reportExpectation(expectation); }
    void reportToBeEqualNumber(const sol::table& expectation) override { reportExpectation(expectation); }
    void reportToBeEqualString(const sol::table& expectation) override { reportExpectation(expectation); }
    void reportToBeInPercentage(const sol::table& expectation) override { reportExpectation(expectation); }
    void reportToBeInRange(const sol::table& expectation) override { reportExpectation(expectation); }
    void reportToBeGreater(const sol::table& expectation) override { reportExpectation(expectation); }
    void reportToBeLesser(const sol::table& expectation) override { reportExpectation(expectation); }
    void reportToBeNear(const sol::table& expectation) override { reportExpectation(expectation); }

private:
    void                  reportExpectation(const sol::table& expectation);
    void                  reportSectionBaseResult(const sol::table& section) const override;
    static nlohmann::json reportSectionTime(const sol::table& section);
    nlohmann::json        m_object  = nlohmann::json::object();
    nlohmann::json*       m_section = nullptr;
};
}    // namespace Frasy::Report::Formatter

#endif    // FRASY_SRC_UTILS_REPORT_FORMATTERS_JSON_H
