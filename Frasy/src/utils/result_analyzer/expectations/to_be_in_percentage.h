/**
 * @file    to_be_in_percentage.h
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

#ifndef FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_IN_PERCENTAGE_H
#define FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_IN_PERCENTAGE_H

#include "to_be_value_base.h"

namespace Frasy::Analyzers {

struct ToBeInPercentageExpectation : public ToBeValueBase {
    ToBeInPercentageExpectation(float expected, float percentage)
    : ToBeValueBase(
        expected, expected - ((expected * percentage) / 100.0f), expected + ((expected * percentage) / 100.0f)),
      Percentage(percentage)
    {
    }
    ~ToBeInPercentageExpectation() override = default;

    void Render() override
    {
        ImGui::BulletText("Expected: To Be In Percentage.");
        ImGui::BulletText("Expected value: %f +/- %0.2f%% [%f, %f]", Expected, Percentage, Min, Max);
        ToBeValueBase::Render();
    }

    nlohmann::json serialize() override
    {
        auto j          = ToBeValueBase::serialize();
        j["type"]       = "to_be_in_percentage";
        j["percentage"] = Percentage;
        return j;
    }

    float Percentage = 0.0f;
};
}    // namespace Frasy::Analyzers

#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_IN_PERCENTAGE_H
