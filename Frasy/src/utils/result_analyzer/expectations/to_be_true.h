/**
 * @file    to_be_true.h
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

#ifndef FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_TRUE_H
#define FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_TRUE_H
#include "../analytic_results.h"

#include <imgui/imgui.h>

namespace Frasy::Analyzers
{
struct ToBeTrueExpectation : public ResultAnalysisResults::Expectation
{
    ~ToBeTrueExpectation() override = default;
    void AddValue(const nlohmann::json& value) override
    {
        Total++;
        if (value.at("pass").get<bool>()) { Passed++; }
    }
    void Render() override
    {
        ImGui::BulletText("Expected: To Be True");
        ImGui::BulletText("Seen: %zu times, Passed: %zu times (%0.2f%%)",
                          Total,
                          Passed,
                          (static_cast<float>(Passed) / static_cast<float>(Total)) * 100.0f);
    }

    size_t Total  = 0;
    size_t Passed = 0;
};
}    // namespace Frasy::Analyzers
#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_TRUE_H
