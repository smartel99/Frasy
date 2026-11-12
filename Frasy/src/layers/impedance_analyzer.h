/**
 * @file    impedance_analyzer.h
 * @brief   Window showing how relevant the DAQ impedance measurements of the last report are.
 *
 * @details It crawls `logs/last/<uut>.json` and looks into the `extra` of every expectation for a table matching
 *          DAQ_ImpedanceResults. Only the ones holding several measures (see `multiple` in daq.lua) can be selected.
 *          For each one it plots the samples of `measures` against the acceptance limits of the expectation, and
 *          computes statistics.
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

#ifndef FRASY_LAYERS_IMPEDANCE_ANALYZER_H
#define FRASY_LAYERS_IMPEDANCE_ANALYZER_H

#include "utils/impedance_analyzer/impedance_scan.h"

#include <Brigerad.h>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Frasy {

class ImpedanceAnalyzer : public Brigerad::Layer {
public:
    ImpedanceAnalyzer() noexcept;
    ~ImpedanceAnalyzer() override = default;

    ImpedanceAnalyzer(const ImpedanceAnalyzer&)            = delete;
    ImpedanceAnalyzer& operator=(const ImpedanceAnalyzer&) = delete;
    ImpedanceAnalyzer(ImpedanceAnalyzer&&)                 = delete;
    ImpedanceAnalyzer& operator=(ImpedanceAnalyzer&&)      = delete;

    void onImGuiRender() override;

    void setVisibility(bool visibility);

private:
    /// An expectation of a test, pointing to its measurement in `LoadedReport::scan`.
    struct Entry {
        std::string label;
        std::size_t measureIndex = 0;
    };
    struct TestNode {
        std::string        name;
        std::vector<Entry> entries;
    };
    struct SequenceNode {
        std::string           name;
        std::vector<TestNode> tests;
    };

    struct LoadedReport {
        std::string                     name;
        std::string                     path;
        std::filesystem::file_time_type lastModified = {};
        ImpedanceAnalysis::ScanResult   scan         = {};    //!< Only the measurements that can be analyzed.
        std::string                     error;               //!< Empty if the report could be loaded.

        std::vector<SequenceNode> sequences;
        std::size_t               sequence = 0;    //!< Selected sequence. The first one by default.
        std::size_t               test     = 0;    //!< Selected test of the sequence.
        std::size_t               entry    = 0;    //!< Selected expectation of the test.

        [[nodiscard]] std::size_t                             getSelectedIndex() const;
        [[nodiscard]] const ImpedanceAnalysis::MeasureReport& getSelected() const;
    };

    static std::vector<LoadedReport> loadReports();
    static LoadedReport              loadReport(const std::filesystem::path& path);
    static void                      buildSelection(LoadedReport& report);
    static void                      restoreSelection(LoadedReport& report, const LoadedReport& previous);

    void refresh();
    bool hasChanged() const;

    void renderReport(LoadedReport& report);
    void renderSelectors(LoadedReport& report);
    void renderMeasure(const ImpedanceAnalysis::MeasureReport& measure);
    void renderIssues(const ImpedanceAnalysis::ImpedanceMeasurement& measurement);
    void renderStats(const ImpedanceAnalysis::Stats& all, const std::optional<ImpedanceAnalysis::Stats>& stable);
    void renderRelevance(const ImpedanceAnalysis::Stats& stats);

    void renderStability(const ImpedanceAnalysis::Stability&       stability,
                         std::size_t                               validSamples,
                         bool                                      hasTimes,
                         const ImpedanceAnalysis::ExcitationCheck& excitation);
    void renderElectricalPlot(const ImpedanceAnalysis::ImpedanceMeasurement& measurement,
                              const std::vector<double>&                     x,
                              const char*                                    xLabel);
    void renderValuePlot(const std::vector<double>&       values,
                         const std::vector<double>&       x,
                         const char*                      xLabel,
                         const ImpedanceAnalysis::Limits& limits,
                         const ImpedanceAnalysis::Stats&  stats,
                         const ImpedanceAnalysis::Stability& stability,
                         std::optional<double>            expectationValue);
    void renderDistributionPlot(const std::vector<double>&       values,
                                const ImpedanceAnalysis::Limits& limits,
                                const ImpedanceAnalysis::Stats&  stats,
                                std::optional<double>            expectationValue);
    void renderSamples(const ImpedanceAnalysis::ImpedanceMeasurement& measurement,
                       const std::vector<double>&                     times);

    std::vector<LoadedReport> m_reports;
    double                    m_lastCheckS = 0.0;
    bool                      m_isVisible  = false;

    /// The plots are navigable (zoom, pan). They only fit their data when something else is displayed.
    std::string m_plotKey;
    int         m_lastPlotFrame = -1;
    bool        m_fitPlots      = true;

    static constexpr const char* s_tag              = "ImpedanceAnalyzer";
    static constexpr const char* s_windowName       = "Impedance Analyzer";
    static constexpr const char* s_lastLogsPath     = "logs/last";
    static constexpr const char* s_logFileExtension = ".json";
    static constexpr double      s_checkPeriodS     = 1.0;
};

}    // namespace Frasy

#endif    // FRASY_LAYERS_IMPEDANCE_ANALYZER_H
