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
#include "../analytic_results.h"

#include <imgui/imgui.h>
#include <json.hpp>
#include <map>

namespace Frasy::Analyzers
{
struct ToBeEqualExpectation : public ResultAnalysisResults::Expectation
{
    ToBeEqualExpectation(const nlohmann::json& expected) : Expected(expected) {}

    void AddValue(const nlohmann::json& value) override
    {
        Total++;
        if (value.at("pass").get<bool>()) { Passed++; }
        Values[value.at("value")]++;
    }
    ~ToBeEqualExpectation() override = default;
    void Render() override
    {
        ImGui::BulletText("Expect: To Be Equal");
        ImGui::BulletText("Seen %zu times, passed %zu times (%0.2f%%)",
                          Total,
                          Passed,
                          (static_cast<float>(Passed) / static_cast<float>(Total)) * 100.0f);
        // TODO dumping every frames like that is stupid.
        ImGui::BulletText("Expected: %s", Expected.dump(2, ' ', true).c_str());
        ImGui::BeginTable("ToBeEqualExpectation", 2);
        ImGui::TableSetupColumn("Value");
        ImGui::TableSetupColumn("Occurrences");
        ImGui::TableHeadersRow();
        for (auto&& [value, count] : Values)
        {
            ImGui::Text("%s", value.dump(2, ' ', true).c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%llu", count);
            ImGui::TableNextColumn();
        }
        ImGui::EndTable();
    }

    size_t                           Total  = 0;
    size_t                           Passed = 0;
    nlohmann::json                   Expected;
    std::map<nlohmann::json, size_t> Values = {};
};
}    // namespace Frasy::Analyzers
#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_EQUAL_H
