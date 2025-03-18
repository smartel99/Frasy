/**
 * @file    keyValue.h
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

#ifndef KEYVALUE_H
#define KEYVALUE_H

#include "formatter.h"
#include <sol/sol.hpp>

namespace Frasy::Report::Formatter {
class KeyValue final : public Formatter {
public:
    KeyValue(std::ofstream& output, const sol::table& result);
    void reportInfo() override;
    void reportUserInfo(const sol::table& table) override;
    void reportIb(const std::string& name) override;
    void reportSequenceResult(const std::string& name) override;
    void reportTestResult(const std::string& name) override;

    static constexpr auto endline = "\n";

protected:
    void reportToBeEqualBoolean(const sol::table& expectation) override;
    void reportToBeEqualNumber(const sol::table& expectation) override;
    void reportToBeEqualString(const sol::table& expectation) override;
    void reportToBeInPercentage(const sol::table& expectation) override;
    void reportToBeInRange(const sol::table& expectation) override;
    void reportToBeGreater(const sol::table& expectation) override;
    void reportToBeLesser(const sol::table& expectation) override;
    void reportToBeNear(const sol::table& expectation) override;

private:
    void        reportSectionBaseResult(const sol::table& section) const override;
    std::string m_sectionPrefix;

    std::ofstream* m_output;
};
};    // namespace Frasy::Report::Formatter

#endif    // KEYVALUE_H
