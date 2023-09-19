/**
 * @file    to_be_lesser.h
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

#ifndef FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_LESSER_H
#define FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_LESSER_H
#include "to_be_value_base.h"

namespace Frasy::Analyzers
{
struct ToBeLesserExpectation : public ToBeValueBase
{
    ToBeLesserExpectation(float max) : ToBeValueBase(max, max, max) {}
    ~ToBeLesserExpectation() override = default;

    void Render() override
    {
        ImGui::BulletText("Expected: To Be Lesser.");
        ImGui::BulletText("Expected value: < %f", Max);
        ToBeValueBase::Render();
    }

    nlohmann::json Serialize() override
    {
        auto j    = ToBeValueBase::Serialize();
        j["type"] = "to_be_lesser";
        return j;
    }
};
}    // namespace Frasy::Analyzers
#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_LESSER_H
