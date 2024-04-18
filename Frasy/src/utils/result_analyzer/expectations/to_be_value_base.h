/**
 * @file    to_be_value_base.h
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

#ifndef FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_VALUE_BASE_H
#define FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_VALUE_BASE_H

#include <imgui/imgui.h>
#include <implot/implot.h>
#include <limits>

namespace Frasy::Analyzers
{

struct ToBeValueBase : public ResultAnalysisResults::Expectation
{
    ToBeValueBase(float expected, float min, float max) : Expected(expected), Min(min), Max(max) {}
    ~ToBeValueBase() override = default;
    void AddValue(const nlohmann::json& value) override
    {
        Total++;
        if (value.at("pass").get<bool>()) { Passed++; }
        float v = value.at("value").get<float>();
        Values.push_back(v);
    }

    void MakeStats() override
    {
        const auto [min, max] = std::minmax_element(Values.begin(), Values.end());
        MinObserved           = *min;
        MaxObserved           = *max;
        std::sort(Values.begin(), Values.end());
        Mean = std::accumulate(Values.begin(), Values.end(), 0.0f);
        Mean /= static_cast<float>(Values.size());

        FindMedian();
        FindMode();
        FindStdDev();
        FindPpPpk();

        Range = MaxObserved - MinObserved;
    }

    void Render() override
    {
        ImGui::BulletText(
          "Seen: %zu times, Passed: %zu times (%0.2f%%)",
          Total,
          Passed,
          (static_cast<float>(Passed) / static_cast<float>(Total)) * 100.0f);

        ShowStats();

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
            ImGui::CheckboxFlags(
              "Horizontal", reinterpret_cast<unsigned int*>(&HistogramFlags), ImPlotHistogramFlags_Horizontal);
            ImGui::SameLine();
            ImGui::CheckboxFlags(
              "Density", reinterpret_cast<unsigned int*>(&HistogramFlags), ImPlotHistogramFlags_Density);
            ImGui::SameLine();
            ImGui::CheckboxFlags(
              "Cumulative", reinterpret_cast<unsigned int*>(&HistogramFlags), ImPlotHistogramFlags_Cumulative);
            ImGui::SameLine();
            ImGui::CheckboxFlags(
              "No Outliers", reinterpret_cast<unsigned int*>(&HistogramFlags), ImPlotHistogramFlags_NoOutliers);

            ImGui::TreePop();
        }

        if (ImPlot::BeginPlot("##Historgram", ImVec2 {-1, 0}, ImPlotFlags_Crosshairs))
        {
            auto xFlags = ImPlotAxisFlags_None;// | ImPlotAxisFlags_AutoFit;
            ImPlot::SetupAxes(nullptr, nullptr, xFlags, ImPlotAxisFlags_AutoFit);
            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);

            ImPlot::PlotHistogram(
              "Values Read", Values.data(), (int)Values.size(), Bins, 1.0, ImPlotRange(), HistogramFlags);
            ImPlot::PlotInfLines("Minimum Accepted", &Min, 1);
            ImPlot::PlotInfLines("Maximum Accepted", &Max, 1);
            ImPlot::EndPlot();
        }
    }

    nlohmann::json serialize() override
    {
        nlohmann::json j  = {};
        j["total"]        = Total;
        j["passed"]       = Passed;
        j["expected"]     = Expected;
        j["min"]          = Min;
        j["max"]          = Max;
        j["values"]       = Values;
        j["min_observed"] = MinObserved;
        j["max_observed"] = MaxObserved;
        j["mean"]         = Mean;
        j["median"]       = Median;
        j["mode"]         = Mode;
        j["range"]        = Range;
        j["std_dev"]      = StdDev;
        j["pp"]           = Pp;
        j["ppk"]          = Ppk;

        return j;
    }

    size_t             Total    = 0;
    size_t             Passed   = 0;
    float              Expected = 0;
    float              Min      = 0;
    float              Max      = 0;
    std::vector<float> Values   = {};

    float MinObserved = std::numeric_limits<float>::max();
    float MaxObserved = std::numeric_limits<float>::lowest();
    float Mean        = 0.0f;
    float Median      = 0.0f;
    float Mode        = {};
    float Range       = 0.0f;
    float StdDev      = 0.0f;

    float Pp  = 0.0f;    //!< Process Performance
    float Ppk = 0.0f;

    ImPlotHistogramFlags HistogramFlags = {};
    int                  Bins           = ImPlotBin_Sqrt;

private:
    void FindMedian()
    {
        if ((Values.size() % 2) == 1)
        {
            // Odd size, median is right at the middle.
            if (Values.size() == 1) { Median = Values[0]; }
            else { Median = Values[(Values.size() / 2) + 1]; }
        }
        else
        {
            // Even size, median is the average of the two values around the middle.
            if (Values.empty()) { Median = 0.0f; }
            else if (Values.size() == 2) { Median = (Values[0] + Values[1]) / 2.0f; }
            else
            {
                size_t idx = (Values.size() / 2);
                Median     = (Values[idx] + Values[idx + 1]) / 2.0f;
            }
        }
    }

    void FindMode()
    {
        // Based of: https://stackoverflow.com/a/19920690
        if (Values.empty()) { return; }
        float current       = Values.front();
        Mode                = current;
        size_t currentCount = 0;
        size_t maxCount     = 0;

        for (auto&& value : Values)
        {
            if (value == current) { currentCount++; }
            else
            {
                if (currentCount > maxCount)
                {
                    maxCount = currentCount;
                    Mode     = current;
                }
                currentCount = 1;
                current      = value;
            }
        }
    }

    void FindStdDev()
    {
        StdDev = std::accumulate(
          Values.begin(),
          Values.end(),
          0.0f,
          [mean = Mean](float tot, float value) { return tot + std::pow(value - mean, 2.0f); });
        StdDev = std::sqrt(StdDev / static_cast<float>(Values.size()));
    }

    void FindPpPpk()
    {
        /**
         * Ppk = min(Cpl, Cpu)
         * Ppl = (mean - minTol) / (3 * StdDev)
         * Ppu = (maxTol - mean) / (3 * StdDev)
         */
        float ppl = (Mean - Min) / (3.0f * StdDev);
        float ppu = (Max - Mean) / (3.0f * StdDev);
        Ppk       = std::min(ppl, ppu);

        /**
         * Pp = (maxTol - minTol) / 6 * StdDev
         */
        Pp = (Max - Min) / (6.0f * StdDev);

        // Cp is the same as Pp, but with more data?
    }

    void ShowStats()
    {
        ImGui::BulletText("Min: %f", MinObserved);
        ImGui::SameLine();
        ImGui::BulletText("Max: %f", MaxObserved);
        ImGui::SameLine();
        ImGui::BulletText("Mean: %f", Mean);

        ImGui::BulletText("Median: %f", Median);
        ImGui::SameLine();
        ImGui::BulletText("Range: %f", Range);
        ImGui::SameLine();
        ImGui::BulletText("Mode: %f", Mode);

        ImGui::BulletText("Standard Deviation: %f", StdDev);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("How spread out the values are in relation to the mean, lower is better.");
        }

        auto ValueWithHint = [](const float& v, std::string_view label, std::string_view hint)
        {
            ImGui::BulletText("%s: %f", label.data(), v);
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", hint.data()); }
        };


        ValueWithHint(Pp, "Pp", "");
        ImGui::SameLine();
        ValueWithHint(
          Ppk,
          "Ppk",
          "How close the process is performing compared to its specification limits and accounting for "
          "the natural variability of the process. Larger is better, negative is bad");
    }
};
}    // namespace Frasy::Analyzers
#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_EXPECTATIONS_TO_BE_VALUE_BASE_H
