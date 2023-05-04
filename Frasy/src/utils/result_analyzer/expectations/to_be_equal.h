/**
 * @file    to_be_equal.h
 * @author  Samuel Martel
 * @date    2023-05-02
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
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#ifndef FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_EQUAL_H
#define FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_EQUAL_H
#include "to_be_exact_base.h"

namespace Frasy::Analyzers
{
struct ToBeEqualExpectation : public ToBeExactBase
{
    ToBeEqualExpectation(const nlohmann::json& expected) : Expected(expected.dump(2, ' ', true)) {}
    ~ToBeEqualExpectation() override = default;

    void Render() override
    {
        ImGui::BulletText("Expect: To Be Equal");
        ImGui::BulletText("Expected: %s", Expected.c_str());
        ToBeExactBase::Render();
    }

    std::string Expected;
};
}    // namespace Frasy::Analyzers
#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_EQUAL_H
