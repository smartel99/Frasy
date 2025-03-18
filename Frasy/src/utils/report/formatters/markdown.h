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

#include "formatter.h"
#include <sol/sol.hpp>

namespace Frasy::Report::Formatter
{
    class Markdown final : public Formatter
    {
    public:
        Markdown(sol::state_view& lua, std::ofstream* output, const sol::table& result);
        void reportInfo() override;
        void reportUserInfo(const sol::table& table);
        void reportIb(const std::string& name) override;
        void reportSequenceResult(const std::string& name) override;
        void reportTestResult(const std::string& name) override;
        void reportToBeEqualBoolean(const sol::table& expectation) override;
        void reportToBeEqualNumber(const sol::table& expectation) override;
        void reportToBeEqualString(const sol::table& expectation) override;
        void reportToBeInPercentage(const sol::table& expectation) override;
        void reportToBeInRange(const sol::table& expectation) override;
        void reportToBeGreater(const sol::table& expectation) override;
        void reportToBeLesser(const sol::table& expectation) override;
        void reportToBeNear(const sol::table& expectation) override;

        static constexpr auto endline = "  \n";

    private:
        void reportSectionBaseResult(const sol::table& section) const override;
        std::ofstream* m_output;
    };
}; // namespace Frasy::Report::Formatter
#endif    // FRASY_REPORT_FORMATTER_MARKDOWN_H
