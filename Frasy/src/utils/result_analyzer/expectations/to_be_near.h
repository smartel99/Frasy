/**
 * @file    to_be_near.h
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

#ifndef FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_NEAR_H
#define FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_NEAR_H
#include "expectations/to_be_value_base.h"
#include "to_be_value_base.h"

namespace Frasy::Analyzers
{
struct ToBeNearExpectation : public ToBeValueBase
{
    ToBeNearExpectation(float expected, float deviation)
    : ToBeValueBase(expected, expected - deviation, expected + deviation), Deviation(deviation)
    {
    }
    ~ToBeNearExpectation() override = default;

    void Render() override
    {
        ImGui::BulletText("Expected: To Be Near.");
        ImGui::BulletText("Expected value: %f +/- %f [%f, %f]", Expected, Deviation, Min, Max);
        ToBeValueBase::Render();
    }

    nlohmann::json serialize() override
    {
        auto j         = ToBeValueBase::serialize();
        j["type"]      = "to_be_near";
        j["deviation"] = Deviation;
        return j;
    }

    float Deviation = 0.0f;
};
}    // namespace Frasy::Analyzers
#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_NEAR_H
