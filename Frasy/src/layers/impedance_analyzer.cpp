/**
 * @file    impedance_analyzer.cpp
 * @brief   Window showing how relevant the DAQ impedance measurements of the last report are.
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this
 * program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "impedance_analyzer.h"

#include <imgui.h>
#include <implot.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <format>
#include <fstream>
#include <iterator>
#include <map>
#include <numeric>
#include <span>
#include <string_view>

namespace Frasy {

using namespace ImpedanceAnalysis;

namespace {

constexpr ImVec4 s_passColor = {0.30f, 0.85f, 0.30f, 1.0f};
constexpr ImVec4 s_failColor = {1.00f, 0.30f, 0.30f, 1.0f};
constexpr ImVec4 s_warnColor = {1.00f, 0.80f, 0.20f, 1.0f};

constexpr float s_statusSpacing    = 24.0f;    //!< Between the stable status and the VIN/VOUT status.
constexpr float s_plotHeight       = 300.0f;
constexpr float s_tableWidthRatio  = 0.35f;    //!< Of the width, for the statistics. The graph next to it takes the rest.
constexpr float s_samplesTableHeight = 200.0f;
constexpr ImPlotLocation s_legendLocation = ImPlotLocation_NorthEast;
constexpr ImPlotLegendFlags s_legendFlags = ImPlotLegendFlags_Outside;    // Next to the plot, not over the data.

bool isReportFile(const std::filesystem::directory_entry& entry, std::string_view extension)
{
    std::error_code ec;
    return entry.is_regular_file(ec) && entry.path().extension() == extension;
}

std::string formatNumber(double value)
{
    return std::isnan(value) ? std::string {"-"} : std::format("{:.6g}", value);
}

std::string formatPercent(double ratio)
{
    return std::isnan(ratio) ? std::string {"-"} : std::format("{:.1f} %", ratio * 100.0);
}

void textColored(const ImVec4& color, const std::string& text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextWrapped("%s", text.c_str());
    ImGui::PopStyleColor();
}

void statRow(const char* label, const std::string& all, const std::string& stable)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(all.c_str());
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(stable.c_str());
}

std::pair<const char*, ImVec4> describeRelevance(Relevance relevance)
{
    switch (relevance) {
        case Relevance::Good:
            return {"Good: the noise is small compared to the acceptance band.", s_passColor};
        case Relevance::Marginal:
            return {"Marginal: the noise takes a large part of the band, results may flip near the limits.",
                    s_warnColor};
        case Relevance::Poor:
            return {"Poor: the noise is wider than the band, a single reading is not meaningful.", s_failColor};
        case Relevance::Unknown:
            break;
    }
    return {"", s_warnColor};    // Not displayed: see renderRelevance.
}

/**
 * @brief Previous and next arrows with a dropdown, like the selectors of the Result Analyzer.
 * @param label    Identifies the selector. Prefix it with `##` to hide it.
 * @param selected Index of the selected item, updated on selection.
 * @param items    Items to choose from. Must not be empty.
 * @param nameOf   Gives the displayed name of an item.
 * @return True if the selection changed.
 */
template<typename Items, typename NameOf>
bool renderSelector(const char* label, std::size_t& selected, const Items& items, NameOf nameOf)
{
    bool changed = false;

    ImGui::PushID(label);
    if (ImGui::ArrowButton("##previous", ImGuiDir_Left) && selected > 0) {
        --selected;
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::ArrowButton("##next", ImGuiDir_Right) && selected + 1 < items.size()) {
        ++selected;
        changed = true;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo(label, nameOf(items[selected]).c_str())) {
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (ImGui::Selectable(nameOf(items[i]).c_str(), selected == i)) {
                changed  = changed || selected != i;
                selected = i;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopID();
    return changed;
}

/// Names of the expectations are not unique, number the ones that are not.
void makeLabelsUnique(std::vector<std::string>& labels)
{
    std::map<std::string, std::size_t> total;
    for (const auto& label : labels) {
        ++total[label];
    }
    std::map<std::string, std::size_t> seen;
    for (auto& label : labels) {
        if (total[label] > 1) { label = std::format("{} ({})", label, ++seen[label]); }
    }
}

}    // namespace

ImpedanceAnalyzer::ImpedanceAnalyzer() noexcept = default;

std::size_t ImpedanceAnalyzer::LoadedReport::getSelectedIndex() const
{
    return sequences.at(sequence).tests.at(test).entries.at(entry).measureIndex;
}

const MeasureReport& ImpedanceAnalyzer::LoadedReport::getSelected() const
{
    return scan.found.at(getSelectedIndex());
}

void ImpedanceAnalyzer::setVisibility(bool visibility)
{
    m_isVisible = visibility;
    if (visibility) {
        refresh();
        ImGui::SetWindowFocus(s_windowName);
    }
}

void ImpedanceAnalyzer::onImGuiRender()
{
    if (!m_isVisible) { return; }

    if (ImGui::Begin(s_windowName, &m_isVisible, ImGuiWindowFlags_NoDocking)) {
        // The reports are rewritten by each run, only look at the disk once in a while.
        if (const double now = ImGui::GetTime(); now - m_lastCheckS >= s_checkPeriodS) {
            m_lastCheckS = now;
            if (hasChanged()) { refresh(); }
        }

        if (m_reports.empty()) {
            ImGui::TextWrapped("No report found in '%s'. Run the tests first.", s_lastLogsPath);
        }
        else if (ImGui::BeginTabBar("ImpedanceAnalyzerTabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
            for (auto& report : m_reports) {
                if (ImGui::BeginTabItem(std::format("UUT {}", report.name).c_str())) {
                    renderReport(report);
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

// --- Loading ---

std::vector<ImpedanceAnalyzer::LoadedReport> ImpedanceAnalyzer::loadReports()
{
    namespace fs = std::filesystem;

    std::vector<fs::path> paths;
    std::error_code       ec;
    if (!fs::exists(s_lastLogsPath, ec)) { return {}; }
    for (const auto& entry : fs::directory_iterator(s_lastLogsPath, ec)) {
        if (isReportFile(entry, s_logFileExtension)) { paths.push_back(entry.path()); }
    }
    std::ranges::sort(paths);

    std::vector<LoadedReport> reports;
    reports.reserve(paths.size());
    for (const auto& path : paths) {
        reports.push_back(loadReport(path));
    }
    return reports;
}

ImpedanceAnalyzer::LoadedReport ImpedanceAnalyzer::loadReport(const std::filesystem::path& path)
{
    LoadedReport report;
    report.name = path.stem().string();
    report.path = path.string();

    std::error_code ec;
    report.lastModified = std::filesystem::last_write_time(path, ec);

    std::ifstream file(path);
    if (!file) {
        report.error = "Unable to open the file.";
        return report;
    }

    // The report is external input: a half written or invalid file must not take the application down.
    try {
        report.scan = scanReport(nlohmann::json::parse(file));
    }
    catch (const std::exception& e) {
        BR_LOG_ERROR(s_tag, "Unable to load '{}': {}", report.path, e.what());
        report.error = e.what();
        return report;
    }

    // Results with a different structure, or with a single measure, have nothing to analyze.
    std::erase_if(report.scan.found, [](const MeasureReport& found) { return !found.measurement.isAnalyzable(); });
    buildSelection(report);
    return report;
}

void ImpedanceAnalyzer::buildSelection(LoadedReport& report)
{
    report.sequences.clear();

    // Group the measurements by sequence then test, in the order of the report.
    for (std::size_t i = 0; i < report.scan.found.size(); ++i) {
        const auto& [expectation, measurement] = report.scan.found[i];

        auto sequence = std::ranges::find(report.sequences, expectation.sequence, &SequenceNode::name);
        if (sequence == report.sequences.end()) {
            sequence = report.sequences.insert(report.sequences.end(), SequenceNode {.name = expectation.sequence});
        }
        auto test = std::ranges::find(sequence->tests, expectation.test, &TestNode::name);
        if (test == sequence->tests.end()) {
            test = sequence->tests.insert(sequence->tests.end(), TestNode {.name = expectation.test});
        }

        // An expectation can hold several results. The path tells which one when it is not `extra` itself.
        std::string label = expectation.name.empty() ? expectation.method : expectation.name;
        if (measurement.path != "extra") { label += std::format(" [{}]", measurement.path); }
        test->entries.push_back({.label = std::move(label), .measureIndex = i});
    }

    for (auto& sequence : report.sequences) {
        for (auto& test : sequence.tests) {
            std::vector<std::string> labels;
            for (const auto& entry : test.entries) {
                labels.push_back(entry.label);
            }
            makeLabelsUnique(labels);
            for (std::size_t i = 0; i < labels.size(); ++i) {
                test.entries[i].label = std::move(labels[i]);
            }
        }
    }
}

void ImpedanceAnalyzer::restoreSelection(LoadedReport& report, const LoadedReport& previous)
{
    if (previous.sequences.empty()) { return; }
    const auto& previousSequence = previous.sequences.at(previous.sequence);
    const auto& previousTest     = previousSequence.tests.at(previous.test);
    const auto& previousEntry    = previousTest.entries.at(previous.entry);

    const auto sequence = std::ranges::find(report.sequences, previousSequence.name, &SequenceNode::name);
    if (sequence == report.sequences.end()) { return; }
    report.sequence = static_cast<std::size_t>(std::distance(report.sequences.begin(), sequence));

    const auto test = std::ranges::find(sequence->tests, previousTest.name, &TestNode::name);
    if (test == sequence->tests.end()) { return; }
    report.test = static_cast<std::size_t>(std::distance(sequence->tests.begin(), test));

    const auto entry = std::ranges::find(test->entries, previousEntry.label, &Entry::label);
    if (entry == test->entries.end()) { return; }
    report.entry = static_cast<std::size_t>(std::distance(test->entries.begin(), entry));
}

bool ImpedanceAnalyzer::hasChanged() const
{
    namespace fs = std::filesystem;

    std::error_code ec;
    if (!fs::exists(s_lastLogsPath, ec)) { return !m_reports.empty(); }

    std::size_t count = 0;
    for (const auto& entry : fs::directory_iterator(s_lastLogsPath, ec)) {
        if (!isReportFile(entry, s_logFileExtension)) { continue; }
        ++count;
        const auto known = std::ranges::find(m_reports, entry.path().string(), &LoadedReport::path);
        if (known == m_reports.end() || known->lastModified != entry.last_write_time(ec)) { return true; }
    }
    return count != m_reports.size();
}

void ImpedanceAnalyzer::refresh()
{
    auto reports = loadReports();
    // Keep what the user was looking at, if it is still there. Otherwise it stays on the first result.
    for (auto& report : reports) {
        const auto previous = std::ranges::find(m_reports, report.path, &LoadedReport::path);
        if (previous != m_reports.end()) { restoreSelection(report, *previous); }
    }
    m_reports = std::move(reports);
}

// --- Rendering ---

void ImpedanceAnalyzer::renderReport(LoadedReport& report)
{
    if (!report.error.empty()) {
        textColored(s_failColor, std::format("Unable to load the report: {}", report.error));
        return;
    }
    if (report.sequences.empty()) {
        ImGui::TextWrapped(
          "No impedance result to analyze in the %zu expectation(s) of this report. Pass the result of "
          "MeasureResistor, MeasureCapacitor or MeasureInductor, run with `multiple`, as `extra` of the expectation: "
          "`Expect(r.value, name, { extra = r })`.",
          report.scan.expectationsScanned);
        return;
    }

    renderSelectors(report);
    ImGui::Separator();

    // Navigating a plot (zoom, pan) must survive the next frames. Fit the data only when another measurement is
    // displayed, or when the plots were not drawn last frame (another tab, window closed).
    const auto key = std::format(
      "{}|{}|{}", report.path, report.getSelectedIndex(), report.lastModified.time_since_epoch().count());
    const int frame = ImGui::GetFrameCount();
    m_fitPlots      = key != m_plotKey || frame != m_lastPlotFrame + 1;
    m_plotKey       = key;
    m_lastPlotFrame = frame;

    if (ImGui::BeginChild("##details", ImVec2 {0.0f, 0.0f}, false)) { renderMeasure(report.getSelected()); }
    ImGui::EndChild();
}

void ImpedanceAnalyzer::renderSelectors(LoadedReport& report)
{
    // The selection is always valid: a change of a level resets the ones below it to their first item.
    const auto name = [](const auto& item) -> const std::string& { return item.name; };
    // The labels are hidden (##): the dropdowns are self explanatory.
    if (renderSelector("##Sequence", report.sequence, report.sequences, name)) {
        report.test  = 0;
        report.entry = 0;
    }

    const auto& sequence = report.sequences.at(report.sequence);
    if (renderSelector("##Test", report.test, sequence.tests, name)) { report.entry = 0; }

    const auto& test = sequence.tests.at(report.test);
    renderSelector("##Expectation", report.entry, test.entries, [](const Entry& entry) -> const std::string& {
        return entry.label;
    });
}

void ImpedanceAnalyzer::renderMeasure(const MeasureReport& measure)
{
    const auto& [expectation, measurement] = measure;

    ImGui::Text("%s - ", expectation.method.c_str());
    ImGui::SameLine();
    textColored(expectation.pass ? s_passColor : s_failColor, expectation.pass ? "PASSED" : "FAILED");
    ImGui::TextWrapped("Expectation value: %s, min: %s, max: %s, expected: %s",
                       expectation.value ? formatNumber(*expectation.value).c_str() : "-",
                       expectation.limits.min ? formatNumber(*expectation.limits.min).c_str() : "-",
                       expectation.limits.max ? formatNumber(*expectation.limits.max).c_str() : "-",
                       expectation.limits.expected ? formatNumber(*expectation.limits.expected).c_str() : "-");

    renderIssues(measurement);
    ImGui::Separator();

    // The expectation limits apply to the expectation's value.
    const auto values = extractChannel(measurement.measures, Channel::Value);
    const auto stats  = computeStats(values, expectation.limits);

    // Without a time stamp (reports from before `time` was added), the position of the sample is used.
    auto       times    = extractTimes(measurement.measures);
    const bool hasTimes = !times.empty();
    if (!hasTimes) {
        times.resize(values.size());
        std::iota(times.begin(), times.end(), 0.0);
    }
    const char* xLabel    = hasTimes ? "Time (ms)" : "Sample";
    const auto  stability = computeStability(values, times);

    // The quality of the measure (noise, spread, ...) is the one once the value is stable: the transient before is
    // not noise. Without stability, there is no such part.
    std::optional<Stats> stableStats;
    if (stability.stable) {
        stableStats = computeStats(std::span<const double> {values}.subspan(stability.firstIndex), expectation.limits);
    }

    renderRelevance(stableStats ? *stableStats : stats);
    renderStability(stability, stats.count - stats.invalidCount, hasTimes, checkExcitation(measurement));

    // Top: the value over time and its distribution. Bottom: the statistics and the electrical signals.
    renderValuePlot(values, times, xLabel, expectation.limits, stats, stability, expectation.value);
    ImGui::SameLine();
    renderDistributionPlot(values, expectation.limits, stats, expectation.value);

    renderStats(stats, stableStats);
    ImGui::SameLine();
    renderElectricalPlot(measurement, times, xLabel);

    renderSamples(measurement, hasTimes ? times : std::vector<double> {});
}

void ImpedanceAnalyzer::renderIssues(const ImpedanceMeasurement& measurement)
{
    for (const auto& issue : measurement.issues) {
        textColored(s_warnColor, std::format("- {}", issue));
    }

    if (ImGui::TreeNode("Parameters")) {
        for (const auto& [key, value] : measurement.parameters.items()) {
            ImGui::TextWrapped("%s: %s", key.c_str(), value.dump().c_str());
        }
        ImGui::TreePop();
    }
}

void ImpedanceAnalyzer::renderStats(const Stats& all, const std::optional<Stats>& stable)
{
    // A child, so that the graph can sit next to the table.
    const float width = ImGui::GetContentRegionAvail().x * s_tableWidthRatio;
    if (ImGui::BeginChild("##statsPanel", ImVec2 {width, s_plotHeight}, false)) {
        if (ImGui::BeginTable(
              "##stats", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("");
            ImGui::TableSetupColumn("All");
            ImGui::TableSetupColumn("Stable");
            ImGui::TableHeadersRow();

            // `cell` gives the text of a statistic. The stable column is empty when the value never gets stable.
            const auto row = [&](const char* label, auto cell) {
                statRow(label, cell(all), stable ? cell(*stable) : std::string {"-"});
            };
            row("Samples", [](const Stats& s) { return std::format("{}", s.count); });
            row("Timeouts (null)", [](const Stats& s) { return std::format("{}", s.invalidCount); });
            row("Mean", [](const Stats& s) { return formatNumber(s.mean); });
            row("Std deviation", [](const Stats& s) { return formatNumber(s.stdDev); });
            row("Min", [](const Stats& s) { return formatNumber(s.min); });
            row("Max", [](const Stats& s) { return formatNumber(s.max); });
            row("Peak to peak", [](const Stats& s) { return formatNumber(s.peakToPeak); });
            row("First / last",
                [](const Stats& s) { return std::format("{} / {}", formatNumber(s.first), formatNumber(s.last)); });
            row("Samples inside limits", [](const Stats& s) { return formatPercent(s.inBandRatio); });
            row("Noise vs band (6 sigma / width)", [](const Stats& s) { return formatPercent(s.bandUsage); });
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}

void ImpedanceAnalyzer::renderRelevance(const Stats& stats)
{
    // Without a verdict (no band on the expectation, too few samples), there is nothing to say.
    if (stats.relevance == Relevance::Unknown) { return; }
    const auto [text, color] = describeRelevance(stats.relevance);
    textColored(color, text);
}

void ImpedanceAnalyzer::renderStability(const Stability&       stability,
                                        std::size_t            validSamples,
                                        bool                   hasTimes,
                                        const ExcitationCheck& excitation)
{
    if (validSamples < s_minStabilitySamples) {
        textColored(
          s_warnColor,
          std::format("Stability: at least {} valid samples are needed, got {}.", s_minStabilitySamples, validSamples));
    }
    else if (stability.stable) {
        const auto delay = hasTimes ? std::format("{:.0f} ms", stability.start)
                                    : std::format("sample {:.0f} (no time in this report)", stability.start);
        textColored(s_passColor,
                    std::format("Stable after {} (within {} of {}).",
                                delay,
                                formatNumber(stability.tolerance),
                                formatNumber(stability.reference)));
    }
    else {
        textColored(
          s_warnColor,
          std::format("Not stable: the value is still moving in the last half of the samples (final value {}, "
                      "tolerance {}).",
                      formatNumber(stability.reference),
                      formatNumber(stability.tolerance)));
    }

    // On the same line: tells if an external signal disturbs the measurement, VIN + VOUT is not at VEXC then.
    ImGui::SameLine(0.0f, s_statusSpacing);
    // The font has no "≈", "~" is its ASCII counterpart.
    textColored(excitation.ok ? s_passColor : s_failColor,
                excitation.ok ? "VEXC ~ VIN + VOUT OK" : "VEXC ~ VIN + VOUT ERROR");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("VEXC = %s V: VIN + VOUT must be between %s V and %s V.\n"
                          "Mean: %s V, furthest from VEXC: %s V (%zu samples).",
                          formatNumber(excitation.expected).c_str(),
                          formatNumber(excitation.min).c_str(),
                          formatNumber(excitation.max).c_str(),
                          formatNumber(excitation.mean).c_str(),
                          formatNumber(excitation.worst).c_str(),
                          excitation.samples);
    }
}

void ImpedanceAnalyzer::renderElectricalPlot(const ImpedanceMeasurement& measurement,
                                             const std::vector<double>&  x,
                                             const char*                 xLabel)
{
    const auto vin        = extractChannel(measurement.measures, Channel::Vin);
    const auto vout       = extractChannel(measurement.measures, Channel::Vout);
    const auto confidence = extractChannel(measurement.measures, Channel::Confidence);
    const auto count      = static_cast<int>(x.size());

    if (m_fitPlots) { ImPlot::SetNextAxesToFit(); }
    if (ImPlot::BeginPlot("Vin / Vout / Confidence", ImVec2 {-1.0f, s_plotHeight})) {
        ImPlot::SetupLegend(s_legendLocation, s_legendFlags);
        ImPlot::SetupAxis(ImAxis_X1, xLabel);
        ImPlot::SetupAxis(ImAxis_Y1, "Vin / Vout");
        ImPlot::SetupAxis(ImAxis_Y2, "Confidence", ImPlotAxisFlags_AuxDefault);

        ImPlot::PlotLine("Vin", x.data(), vin.data(), count);
        ImPlot::PlotLine("Vout", x.data(), vout.data(), count);

        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
        ImPlot::PlotLine("Confidence", x.data(), confidence.data(), count);
        ImPlot::EndPlot();
    }
}

void ImpedanceAnalyzer::renderValuePlot(const std::vector<double>& values,
                                        const std::vector<double>& x,
                                        const char*                xLabel,
                                        const Limits&              limits,
                                        const Stats&               stats,
                                        const Stability&           stability,
                                        std::optional<double>      expectationValue)
{
    const auto count = static_cast<int>(values.size());
    // Half of the row. The distribution takes the rest.
    const float width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

    if (m_fitPlots) { ImPlot::SetNextAxesToFit(); }
    if (ImPlot::BeginPlot("Value", ImVec2 {width, s_plotHeight})) {
        ImPlot::SetupLegend(s_legendLocation, s_legendFlags);
        ImPlot::SetupAxis(ImAxis_X1, xLabel);
        ImPlot::SetupAxis(ImAxis_Y1, "Value");

        ImPlot::PlotLine("Value", x.data(), values.data(), count);
        ImPlot::PlotScatter("Points", x.data(), values.data(), count);
        if (limits.min) { ImPlot::PlotInfLines("Minimum accepted", &*limits.min, 1, ImPlotInfLinesFlags_Horizontal); }
        if (limits.max) { ImPlot::PlotInfLines("Maximum accepted", &*limits.max, 1, ImPlotInfLinesFlags_Horizontal); }
        if (!std::isnan(stats.mean)) { ImPlot::PlotInfLines("Mean", &stats.mean, 1, ImPlotInfLinesFlags_Horizontal); }
        if (expectationValue) {
            ImPlot::PlotInfLines("Expectation value", &*expectationValue, 1, ImPlotInfLinesFlags_Horizontal);
        }
        if (stability.stable) { ImPlot::PlotInfLines("Stable from", &stability.start, 1); }
        ImPlot::EndPlot();
    }
}

void ImpedanceAnalyzer::renderDistributionPlot(const std::vector<double>& values,
                                               const Limits&              limits,
                                               const Stats&               stats,
                                               std::optional<double>      expectationValue)
{
    std::vector<double> valid;
    valid.reserve(values.size());
    std::ranges::copy_if(values, std::back_inserter(valid), [](double value) { return std::isfinite(value); });

    if (m_fitPlots) { ImPlot::SetNextAxesToFit(); }
    if (ImPlot::BeginPlot("Distribution", ImVec2 {-1.0f, s_plotHeight})) {
        ImPlot::SetupLegend(s_legendLocation, s_legendFlags);
        ImPlot::SetupAxis(ImAxis_X1, "Value");
        ImPlot::SetupAxis(ImAxis_Y1, "Count");

        if (valid.size() >= 2) {
            ImPlot::PlotHistogram(
              "Distribution", valid.data(), static_cast<int>(valid.size()), ImPlotBin_Sturges);
        }
        // The same markers as the value graph, vertical here since the value is on the X axis.
        if (limits.min) { ImPlot::PlotInfLines("Minimum accepted", &*limits.min, 1); }
        if (limits.max) { ImPlot::PlotInfLines("Maximum accepted", &*limits.max, 1); }
        if (!std::isnan(stats.mean)) { ImPlot::PlotInfLines("Mean", &stats.mean, 1); }
        if (expectationValue) { ImPlot::PlotInfLines("Expectation value", &*expectationValue, 1); }
        ImPlot::EndPlot();
    }
}

void ImpedanceAnalyzer::renderSamples(const ImpedanceMeasurement& measurement, const std::vector<double>& times)
{
    if (!ImGui::CollapsingHeader("Samples")) { return; }

    const bool hasTimes = !times.empty();
    if (ImGui::BeginTable("##samples",
                          hasTimes ? 3 : 2,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                            ImGuiTableFlags_SizingStretchProp,
                          ImVec2 {0.0f, s_samplesTableHeight})) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("#");
        if (hasTimes) { ImGui::TableSetupColumn("Time (ms)"); }
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();
        for (std::size_t i = 0; i < measurement.measures.size(); ++i) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%zu", i);
            if (hasTimes) {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(formatNumber(times[i]).c_str());
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(formatNumber(measurement.measures[i].value).c_str());
        }
        ImGui::EndTable();
    }
}

}    // namespace Frasy
