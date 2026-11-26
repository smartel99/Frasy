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

#include "utils/report/result.h"

#include <json.hpp>

#include <array>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace Frasy::Analyzers {
using Report::ResultCounts;

struct ResultAnalysisResults {
    struct Expectation {
        virtual ~Expectation() = default;
        std::string Name;
        /** @param passed Whether this run of the expectation passed, read from the report's layout. */
        virtual void           AddValue(const nlohmann::json& value, bool passed) = 0;
        virtual void           MakeStats()                                        = 0;
        virtual void           Render()                                           = 0;
        virtual nlohmann::json serialize()                                        = 0;
    };
    struct Test {
        std::string                                         Name;
        ResultCounts                                        Results         = {};
        std::vector<double>                                 Durations       = {};
        double                                              AverageDuration = 0.0;
        std::map<std::string, std::shared_ptr<Expectation>> Expectations    = {};
    };
    struct Sequence {
        std::string                 Name;
        ResultCounts                Results         = {};
        std::vector<double>         Durations       = {};
        double                      AverageDuration = 0.0;
        std::map<std::string, Test> Tests           = {};
    };
    struct Location {
        std::string                     Name;
        std::string                     Version;
        ResultCounts                    Results         = {};
        std::vector<double>             Durations       = {};
        double                          AverageDuration = 0.0;
        std::map<std::string, Sequence> Sequences       = {};
    };

    std::map<std::string, Location> Locations = {};
};
}    // namespace Frasy::Analyzers

#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_ANALYTIC_RESULTS_H
