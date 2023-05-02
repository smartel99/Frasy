/**
 * @file    to_be_type.h
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

#ifndef FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_TYPE_H
#define FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_TYPE_H
#include "../analytic_results.h"

#include <exception>
#include <imgui/imgui.h>

namespace Frasy::Analyzers
{
struct ToBeTypeExpectation : public ResultAnalysisResults::Expectation
{
    ToBeTypeExpectation(const std::string& expected) : Expected(expected) {}
    ~ToBeTypeExpectation() override = default;

    void AddValue(const nlohmann::json& value) override
    {
        Total++;
        if (value.at("pass").get<bool>()) { Passed++; }
        Values[value]++;
    }
    void Render() override
    {
        ImGui::BulletText("Expect: To Be Type");
        ImGui::BulletText("Seen %zu times, passed %zu times (%0.2f%%)",
                          Total,
                          Passed,
                          (static_cast<float>(Passed) / static_cast<float>(Total)) * 100.0f);
        ImGui::BulletText("Expected type: %s", Expected.c_str());
        // TODO
        //        ImGui::BeginTable("ToBeType", 2);
        //        ImGui::TableSetupColumn("Value");
        //        ImGui::TableSetupColumn("Occurrences");
        //        ImGui::TableHeadersRow();
        //        for (auto&& [value, count] : Values)
        //        {
        //            ImGui::Text("%s", value.c_str());
        //            ImGui::TableNextColumn();
        //            ImGui::Text("%llu", count);
        //            ImGui::TableNextColumn();
        //        }
        //        ImGui::EndTable();
    }

    size_t                           Total  = 0;
    size_t                           Passed = 0;
    std::string                      Expected;
    std::map<nlohmann::json, size_t> Values;
};
}    // namespace Frasy::Analyzers
#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_TYPE_H
