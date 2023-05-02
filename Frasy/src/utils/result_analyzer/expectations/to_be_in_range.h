/**
 * @file    to_be_in_range.h
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

#ifndef FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_IN_RANGE_H
#define FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_IN_RANGE_H
#include "../analytic_results.h"

#include <imgui/imgui.h>
#include <implot/implot.h>

namespace Frasy::Analyzers
{
struct ToBeInRangeExpectation : public ResultAnalysisResults::Expectation
{
    ToBeInRangeExpectation(float min, float max) : Min(min), Max(max) {}
    ~ToBeInRangeExpectation() override = default;

    void AddValue(const nlohmann::json& value) override
    {
        Total++;
        if (value.at("pass").get<bool>()) { Passed++; }
        Values.push_back(value.at("value").get<float>());
    }

    void Render() override
    {
        ImGui::BulletText("Expected: To Be In Percentage.");
        ImGui::BulletText("Seen: %zu times, Passed: %zu times (%0.2f%%)",
                          Total,
                          Passed,
                          (static_cast<float>(Passed) / static_cast<float>(Total)) * 100.0f);
        ImGui::BulletText("Expected value: [%f, %f]", Min, Max);

        if (ImGui::TreeNode("Settings"))
        {
            ImGui::SetNextItemWidth(200);
            if (ImGui::RadioButton("Sqrt", Bins == ImPlotBin_Sqrt)) { Bins = ImPlotBin_Sqrt; }
            ImGui::SameLine();
            if (ImGui::RadioButton("Sturges", Bins == ImPlotBin_Sturges)) { Bins = ImPlotBin_Sturges; }
            ImGui::SameLine();
            if (ImGui::RadioButton("Rice", Bins == ImPlotBin_Rice)) { Bins = ImPlotBin_Rice; }
            ImGui::SameLine();
            if (ImGui::RadioButton("Scott", Bins == ImPlotBin_Scott)) { Bins = ImPlotBin_Scott; }
            ImGui::SameLine();
            if (ImGui::RadioButton("N Bins", Bins >= 0)) { Bins = 50; }
            if (Bins >= 0)
            {
                ImGui::SameLine();
                ImGui::SetNextItemWidth(200);
                ImGui::SliderInt("##Bins", &Bins, 1, 100);
            }
            ImGui::CheckboxFlags("Horizontal", (unsigned int*)&HistogramFlags, ImPlotHistogramFlags_Horizontal);
            ImGui::SameLine();
            ImGui::CheckboxFlags("Density", (unsigned int*)&HistogramFlags, ImPlotHistogramFlags_Density);
            ImGui::SameLine();
            ImGui::CheckboxFlags("Cumulative", (unsigned int*)&HistogramFlags, ImPlotHistogramFlags_Cumulative);
            ImGui::SameLine();
            ImGui::CheckboxFlags("No Outliers", (unsigned int*)&HistogramFlags, ImPlotHistogramFlags_NoOutliers);

            ImGui::TreePop();
        }

        if (ImPlot::BeginPlot("##Historgram"))
        {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
            ImPlot::PlotHistogram(
              "Values Read", Values.data(), Values.size(), Bins, 1.0, ImPlotRange(), HistogramFlags);
            ImPlot::EndPlot();
        }
    }

    size_t             Total  = 0;
    size_t             Passed = 0;
    float              Min    = 0.0f;
    float              Max    = 0.0f;
    std::vector<float> Values;

    ImPlotHistogramFlags HistogramFlags = {};
    int                  Bins           = 50;
};
}    // namespace Frasy::Analyzers
#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_IN_RANGE_H
