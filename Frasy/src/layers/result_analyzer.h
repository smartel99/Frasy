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

#include <Brigerad.h>
#include <thread>
#include <vector>

namespace Frasy
{
class ResultAnalyzer : public Brigerad::Layer
{
public:
    ResultAnalyzer() noexcept;
    ~ResultAnalyzer() override = default;

    void OnImGuiRender() override;

    void SetVisibility(bool visibility);

private:
    static void RenderStringList(std::string_view                   name,
                                 std::string_view                   tooltip,
                                 std::vector<std::array<char, 32>>& strings);
    void        RenderAnalysisResults();
    void        RenderLocationAnalysisResults(const Analyzers::ResultAnalysisResults::Location& location);
    void        RenderSequenceAnalysisResults(const Analyzers::ResultAnalysisResults::Sequence& sequence);
    void        RenderTestAnalysisResults(const Analyzers::ResultAnalysisResults::Test& test);

private:
    bool                         m_isVisible  = false;
    static constexpr const char* s_windowName = "Result Analyzer Options";

    Analyzers::ResultOptions         m_options       = {};
    bool                             m_renderResults = false;
    Analyzers::ResultAnalysisResults m_results       = {};

    Analyzers::ResultAnalyzer m_analyzer;
    bool                      m_generating     = false;
    bool                      m_doneGenerating = false;
    std::thread               m_generatorThread;
};
}    // namespace Frasy

#endif    // FRASY_SRC_LAYERS_RESULT_ANALYZER_H
