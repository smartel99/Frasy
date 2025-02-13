/**
 * @file    analyzer.h
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

#ifndef FRASY_SRC_UTILS_RESULT_ANALYZER_ANALYZER_H
#define FRASY_SRC_UTILS_RESULT_ANALYZER_ANALYZER_H

#include "analytic_results.h"
#include "options.h"

#include <json.hpp>

namespace Frasy::Analyzers
{
class ResultAnalyzer
{
public:
    ResultAnalyzer() = default;
    ResultAnalyzer(const ResultOptions& options) : m_options(options) {}

    ResultAnalysisResults Analyze(const std::string& title);

    size_t ToAnalyze = 0;
    size_t Analyzed = 0;

private:
    void AnalyzeFile(const std::string& path);
    void AnalyzeSequence(const nlohmann::json& sequence, ResultAnalysisResults::Sequence& results);
    void AnalyzeTest(const nlohmann::json& test, ResultAnalysisResults::Test& results);
    void AnalyzeExpectation(const nlohmann::json&                               expectation,
                            std::shared_ptr<ResultAnalysisResults::Expectation>& results);

    std::shared_ptr<ResultAnalysisResults::Expectation> MakeExpectationFromDetails(const nlohmann::json& expectation);

private:
    ResultOptions         m_options = {};
    ResultAnalysisResults m_results = {};
};
}    // namespace Frasy::Analyzers

#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_ANALYZER_H
