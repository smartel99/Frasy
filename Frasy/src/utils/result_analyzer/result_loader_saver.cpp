/**
 * @file    result_loader_saver.cpp
 * @author  Samuel Martel
 * @date    2023-05-08
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
#include "result_loader_saver.h"

#include "analytic_results.h"
#include "expectations/to_be_equal.h"
#include "expectations/to_be_exact_base.h"
#include "expectations/to_be_false.h"
#include "expectations/to_be_in_percentage.h"
#include "expectations/to_be_in_range.h"
#include "expectations/to_be_near.h"
#include "expectations/to_be_true.h"
#include "expectations/to_be_type.h"
#include "expectations/to_be_value_base.h"

#include <Brigerad/Debug/Instrumentor.h>
#include <fstream>
#include <json.hpp>

namespace Frasy::Analyzers
{

namespace
{
nlohmann::json LoadJson(const std::string& path)
{
    BR_PROFILE_FUNCTION();
    std::ifstream j(path);

    std::string fullFile;
    std::string line;

    while (std::getline(j, line)) { fullFile += line; }
    j.close();

    return nlohmann::json::parse(fullFile);
}

void LoadToBeValueExpectation(std::shared_ptr<ResultAnalysisResults::Expectation>& expectation,
                              const nlohmann::json&                                data)
{
    ToBeValueBase* ptr = static_cast<ToBeValueBase*>(expectation.get());
    ptr->Total         = data.at("total").get<size_t>();
    ptr->Passed        = data.at("passed").get<size_t>();
    ptr->Expected      = data.at("expected").get<float>();
    ptr->Min           = data.at("min").get<float>();
    ptr->Max           = data.at("max").get<float>();
    ptr->Values        = data.at("values").get<std::vector<float>>();
    ptr->MinObserved   = data.at("min_observed").get<float>();
    ptr->MaxObserved   = data.at("max_observed").get<float>();
    ptr->Mean          = data.at("mean").get<float>();
    ptr->Median        = data.at("median").get<float>();
    ptr->Mode          = data.at("mode").get<float>();
    ptr->Range         = data.at("range").get<float>();
    ptr->StdDev        = data.at("std_dev").get<float>();
    ptr->Pp            = data.at("pp").get<float>();
    ptr->Ppk           = data.at("ppk").get<float>();
}

std::shared_ptr<ResultAnalysisResults::Expectation> LoadToBeInPercentageExpectation(const nlohmann::json& data)
{
    auto* exp = new ToBeInPercentageExpectation(data.at("expected").get<float>(), data.at("percentage").get<float>());
    auto  ptr = std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    LoadToBeValueExpectation(ptr, data);
    return ptr;
}

std::shared_ptr<ResultAnalysisResults::Expectation> LoadToBeInRangeExpectation(const nlohmann::json& data)
{
    auto* exp = new ToBeInRangeExpectation(data.at("min").get<float>(), data.at("max").get<float>());
    auto  ptr = std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    LoadToBeValueExpectation(ptr, data);
    return ptr;
}

std::shared_ptr<ResultAnalysisResults::Expectation> LoadToBeNearExpectation(const nlohmann::json& data)
{
    auto* exp = new ToBeNearExpectation(data.at("expected").get<float>(), data.at("deviation").get<float>());
    auto  ptr = std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    LoadToBeValueExpectation(ptr, data);
    return ptr;
}

void LoadToBeExactExpectation(std::shared_ptr<ResultAnalysisResults::Expectation>& expectation,
                              const nlohmann::json&                                data)
{
    ToBeExactBase* ptr = static_cast<ToBeExactBase*>(expectation.get());
    ptr->Total         = data.at("total").get<size_t>();
    ptr->Passed        = data.at("passed").get<size_t>();

    for (auto&& [name, value] : data.at("values").items())
    {
        ptr->Values[name] = {
          .Type   = value.at("type").get<std::string>(),
          .Passed = value.at("passed").get<bool>(),
          .Seen   = value.at("seen").get<size_t>(),
        };
    }
}

std::shared_ptr<ResultAnalysisResults::Expectation> LoadToBeEqualExpectation(const nlohmann::json& data)
{
    auto* exp = new ToBeEqualExpectation(data.at("expected"));
    auto  ptr = std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    LoadToBeExactExpectation(ptr, data);
    return ptr;
}

std::shared_ptr<ResultAnalysisResults::Expectation> LoadToBeFalseExpectation(const nlohmann::json& data)
{
    auto* exp = new ToBeFalseExpectation();
    auto  ptr = std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    LoadToBeExactExpectation(ptr, data);
    return ptr;
}

std::shared_ptr<ResultAnalysisResults::Expectation> LoadToBeTrueExpectation(const nlohmann::json& data)
{
    auto* exp = new ToBeTrueExpectation();
    auto  ptr = std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    LoadToBeExactExpectation(ptr, data);
    return ptr;
}

std::shared_ptr<ResultAnalysisResults::Expectation> LoadToBeTypeExpectation(const nlohmann::json& data)
{
    auto* exp = new ToBeTypeExpectation(data.at("expected").get<std::string>());
    auto  ptr = std::shared_ptr<ResultAnalysisResults::Expectation>(exp);
    LoadToBeExactExpectation(ptr, data);
    return ptr;
}


std::shared_ptr<ResultAnalysisResults::Expectation> LoadExpectation(const nlohmann::json& data)
{
    using namespace std::string_view_literals;
    std::string_view type = data.at("type").get<std::string_view>();

    if (type == "to_be_equal"sv) { return LoadToBeEqualExpectation(data); }
    if (type == "to_be_false"sv) { return LoadToBeFalseExpectation(data); }
    if (type == "to_be_in_percentage"sv) { return LoadToBeInPercentageExpectation(data); }
    if (type == "to_be_in_range"sv) { return LoadToBeInRangeExpectation(data); }
    if (type == "to_be_near"sv) { return LoadToBeNearExpectation(data); }
    if (type == "to_be_true"sv) { return LoadToBeTrueExpectation(data); }
    if (type == "to_be_type"sv) { return LoadToBeTypeExpectation(data); }
    throw std::runtime_error(std::format("Invalid expectation type '{}'", type));
}

ResultAnalysisResults::Test LoadTest(const nlohmann::json& data)
{
    return ResultAnalysisResults::Test {
      .Name            = data.at("name").get<std::string>(),
      .Passed          = data.at("passed").get<size_t>(),
      .PassedPercent   = data.at("passed_percent").get<float>(),
      .Enabled         = data.at("enabled").get<size_t>(),
      .EnabledPercent  = data.at("enabled_percent").get<float>(),
      .Skipped         = data.at("skipped").get<size_t>(),
      .SkippedPercent  = data.at("skipped_percent").get<float>(),
      .Total           = data.at("total").get<size_t>(),
      .Durations       = data.at("durations").get<std::vector<double>>(),
      .AverageDuration = data.at("average_duration").get<double>(),
      .Expectations =
        [&data]()
      {
          std::map<std::string, std::shared_ptr<ResultAnalysisResults::Expectation>> expectations;

          for (auto&& [name, expectation] : data.at("expectations").items())
          {
              expectations[name] = LoadExpectation(expectation);
          }
          return expectations;
      }(),
    };
}

ResultAnalysisResults::Sequence LoadSequence(const nlohmann::json& data)
{
    return ResultAnalysisResults::Sequence {
      .Name            = data.at("name").get<std::string>(),
      .Passed          = data.at("passed").get<size_t>(),
      .PassedPercent   = data.at("passed_percent").get<float>(),
      .Enabled         = data.at("enabled").get<size_t>(),
      .EnabledPercent  = data.at("enabled_percent").get<float>(),
      .Skipped         = data.at("skipped").get<size_t>(),
      .SkippedPercent  = data.at("skipped_percent").get<float>(),
      .Total           = data.at("total").get<size_t>(),
      .Durations       = data.at("durations").get<std::vector<double>>(),
      .AverageDuration = data.at("average_duration").get<double>(),
      .Tests =
        [&data]()
      {
          std::map<std::string, ResultAnalysisResults::Test> tests;

          for (auto&& [name, test] : data.at("tests").items()) { tests[name] = LoadTest(test); }
          return tests;
      }(),
    };
}

ResultAnalysisResults::Location LoadLocation(const nlohmann::json& data)
{
    return ResultAnalysisResults::Location {
      .Name            = data.at("name").get<std::string>(),
      .Version         = data.at("version").get<std::string>(),
      .Passed          = data.at("passed").get<size_t>(),
      .PassedPercent   = data.at("passed_percent").get<float>(),
      .Total           = data.at("total").get<size_t>(),
      .Durations       = data.at("durations").get<std::vector<double>>(),
      .AverageDuration = data.at("average_duration").get<double>(),
      .Sequences =
        [&data]()
      {
          std::map<std::string, ResultAnalysisResults::Sequence> sequences = {};
          for (auto&& [name, sequence] : data.at("sequences").items()) { sequences[name] = LoadSequence(sequence); }

          return sequences;
      }(),
    };
}

}    // namespace

ResultAnalysisResults Load(const std::string& path)
{
    auto                  j       = LoadJson(path);
    ResultAnalysisResults results = {};

    for (auto&& [name, location] : j.items()) { results.Locations[name] = LoadLocation(location); }

    return results;
}

namespace
{
nlohmann::json SaveTest(const ResultAnalysisResults::Test& test)
{
    BR_PROFILE_FUNCTION();
    nlohmann::json j      = {};
    j["name"]             = test.Name;
    j["passed"]           = test.Passed;
    j["passed_percent"]   = test.PassedPercent;
    j["enabled"]          = test.Enabled;
    j["enabled_percent"]  = test.EnabledPercent;
    j["skipped"]          = test.Skipped;
    j["skipped_percent"]  = test.SkippedPercent;
    j["total"]            = test.Total;
    j["durations"]        = test.Durations;
    j["average_duration"] = test.AverageDuration;
    j["expectations"]     = {};
    for (auto&& [name, expectation] : test.Expectations) { j["expectations"][name] = expectation->Serialize(); }

    return j;
}

nlohmann::json SaveSequence(const ResultAnalysisResults::Sequence& sequence)
{
    BR_PROFILE_FUNCTION();
    nlohmann::json j      = {};
    j["name"]             = sequence.Name;
    j["passed"]           = sequence.Passed;
    j["passed_percent"]   = sequence.PassedPercent;
    j["enabled"]          = sequence.Enabled;
    j["enabled_percent"]  = sequence.EnabledPercent;
    j["skipped"]          = sequence.Skipped;
    j["skipped_percent"]  = sequence.SkippedPercent;
    j["total"]            = sequence.Total;
    j["durations"]        = sequence.Durations;
    j["average_duration"] = sequence.AverageDuration;
    j["tests"]            = {};
    for (auto&& [name, test] : sequence.Tests) { j["tests"][name] = SaveTest(test); }

    return j;
}

nlohmann::json SaveLocation(const ResultAnalysisResults::Location& location)
{
    BR_PROFILE_FUNCTION();
    nlohmann::json j      = {};
    j["name"]             = location.Name;
    j["version"]          = location.Version;
    j["passed"]           = location.Passed;
    j["passed_percent"]   = location.PassedPercent;
    j["total"]            = location.Total;
    j["durations"]        = location.Durations;
    j["average_duration"] = location.AverageDuration;
    j["sequences"]        = {};
    for (auto&& [name, sequence] : location.Sequences) { j["sequences"][name] = SaveSequence(sequence); }

    return j;
}
}    // namespace

void Save(const ResultAnalysisResults& results, const std::string& path)
{
    BR_PROFILE_FUNCTION();
    nlohmann::json j = {};

    for (auto&& [name, location] : results.Locations) { j[name] = SaveLocation(location); }

    std::ofstream file = std::ofstream(path);

    file << j.dump(2, ' ', true, nlohmann::detail::error_handler_t::replace);
}
}    // namespace Frasy::Analyzers
