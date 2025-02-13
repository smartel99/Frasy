/**
 * @file    result_analyzer.h
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

#ifndef FRASY_SRC_LAYERS_RESULT_ANALYZER_H
#define FRASY_SRC_LAYERS_RESULT_ANALYZER_H

#include "utils/result_analyzer/analytic_results.h"
#include "utils/result_analyzer/analyzer.h"
#include "utils/result_analyzer/options.h"

#include <array>
#include <Brigerad.h>
#include <functional>
#include <map>
#include <thread>
#include <vector>

namespace Frasy {
class ResultAnalyzer : public Brigerad::Layer {
public:
    ResultAnalyzer() noexcept;
    ~ResultAnalyzer() override = default;

    void onImGuiRender() override;

    void setVisibility(bool visibility);
    void setGetTitle(const std::function<std::string()>& getTitle) { m_getTitle = getTitle; }

private:
    static void renderStringList(std::string_view                   name,
                                 std::string_view                   tooltip,
                                 std::vector<std::array<char, 32>>& strings);
    void        renderAnalysisResults();
    void        renderSingleAnalysisResults();
    void        renderMultipleAnalysisResults();
    void        renderAnalysisResultsFile(const Analyzers::ResultAnalysisResults& results);
    void        renderLocationAnalysisResults(const Analyzers::ResultAnalysisResults::Location& location);
    void        renderSequenceAnalysisResults(const Analyzers::ResultAnalysisResults::Sequence& sequence);
    void        renderTestAnalysisResults(const Analyzers::ResultAnalysisResults::Test& test);

    bool                         m_isVisible  = false;
    static constexpr const char* s_windowName = "Result Analyzer Options";

    Analyzers::ResultOptions                                m_options       = {};
    bool                                                    m_renderResults = false;
    Analyzers::ResultAnalysisResults                        m_lastResults   = {};
    std::map<std::string, Analyzers::ResultAnalysisResults> m_loadedResults = {};

    Analyzers::ResultAnalyzer    m_analyzer;
    bool                         m_generating     = false;
    bool                         m_doneGenerating = false;
    bool                         m_hasGenerated   = false;
    std::thread                  m_generatorThread;
    std::function<std::string()> m_getTitle = []() { return "untitled"; };
};
}    // namespace Frasy

#endif    // FRASY_SRC_LAYERS_RESULT_ANALYZER_H
