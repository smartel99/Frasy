/**
 * @file    analytic_results.h
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

#ifndef FRASY_SRC_UTILS_RESULT_ANALYZER_ANALYTIC_RESULTS_H
#define FRASY_SRC_UTILS_RESULT_ANALYZER_ANALYTIC_RESULTS_H

#include <json.hpp>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace Frasy::Analyzers
{
struct ResultAnalysisResults
{
    struct Expectation
    {
        virtual ~Expectation()                                       = default;
        virtual void           AddValue(const nlohmann::json& value) = 0;
        virtual void           MakeStats()                           = 0;
        virtual void           Render()                              = 0;
        virtual nlohmann::json Serialize()                           = 0;
    };
    struct Test
    {
        std::string                                         Name;
        size_t                                              Passed          = 0;
        float                                               PassedPercent   = 0.0f;
        size_t                                              Enabled         = 0;
        float                                               EnabledPercent  = 0.0f;
        size_t                                              Skipped         = 0;
        float                                               SkippedPercent  = 0.0f;
        size_t                                              Total           = 0;
        std::vector<double>                                 Durations       = {};
        double                                              AverageDuration = 0.0;
        std::map<std::string, std::shared_ptr<Expectation>> Expectations    = {};
    };
    struct Sequence
    {
        std::string                 Name;
        size_t                      Passed          = 0;
        float                       PassedPercent   = 0.0f;
        size_t                      Enabled         = 0;
        float                       EnabledPercent  = 0.0f;
        size_t                      Skipped         = 0;
        float                       SkippedPercent  = 0.0f;
        size_t                      Total           = 0;
        std::vector<double>         Durations       = {};
        double                      AverageDuration = 0.0;
        std::map<std::string, Test> Tests           = {};
    };
    struct Location
    {
        std::string                     Name;
        std::string                     Version;
        size_t                          Passed          = 0;
        float                           PassedPercent   = 0.0f;
        size_t                          Total           = 0;
        std::vector<double>             Durations       = {};
        double                          AverageDuration = 0.0;
        std::map<std::string, Sequence> Sequences       = {};
    };

    std::map<std::string, Location> Locations = {};
};
}    // namespace Frasy::Analyzers

#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_ANALYTIC_RESULTS_H
