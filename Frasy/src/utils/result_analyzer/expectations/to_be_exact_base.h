/**
 * @file    to_be_exact_base.h
 * @author  Samuel Martel
 * @date    2023-05-03
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

#ifndef FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_EXACT_BASE_H
#define FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_EXACT_BASE_H
#include "../analytic_results.h"
#include "json.hpp"

#include <exception>
#include <imgui.h>

namespace Frasy::Analyzers {
struct ToBeExactBase : public ResultAnalysisResults::Expectation {
private:
    struct ObservedValue {
        std::string Type   = {};
        bool        Passed = false;
        size_t      Seen   = 0;
    };

public:
    ~ToBeExactBase() override = default;

    void AddValue(const nlohmann::json& value) override
    {
        Total++;
        if (value.at("pass").get<bool>()) { Passed++; }

        std::string valueStr = value.at("value").dump(2, ' ', true);
        if (!Values.contains(valueStr)) {
            // First time seen, add to the list.
            Values[valueStr] = {
              .Type   = value["expected"].type_name(),
              .Passed = value.at("pass").get<bool>(),
            };
        }
        Values[valueStr].Seen++;
    }

    void MakeStats() override {}

    void Render() override
    {
        ImGui::BulletText("Seen %zu times, passed %zu times (%0.2f%%)",
                          Total,
                          Passed,
                          (static_cast<float>(Passed) / static_cast<float>(Total)) * 100.0f);
        ImGui::BeginTable("ToBeType", 4);
        ImGui::TableSetupColumn("Value");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Passed");
        ImGui::TableSetupColumn("Occurrences");
        ImGui::TableHeadersRow();
        for (auto&& [name, info] : Values) {
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", name.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%s", info.Type.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%s", info.Passed ? "True" : "False");
            ImGui::TableNextColumn();
            ImGui::Text("%zu", info.Seen);
        }
        ImGui::EndTable();
    }

    nlohmann::json serialize() override
    {
        nlohmann::json j = {};
        j["total"]       = Total;
        j["passed"]      = Passed;
        j["values"]      = {};
        for (auto&& [name, value] : Values) {
            j["values"][name]           = {};
            j["values"][name]["type"]   = value.Type;
            j["values"][name]["passed"] = value.Passed;
            j["values"][name]["seen"]   = value.Seen;
        }

        return j;
    }

    size_t                               Total  = 0;
    size_t                               Passed = 0;
    std::map<std::string, ObservedValue> Values;
};
}    // namespace Frasy::Analyzers
#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_EXACT_BASE_H
