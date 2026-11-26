#include "utils/report/result.h"
#include "utils/result_analyzer/analyzer.h"
#include "utils/result_analyzer/result_loader_saver.h"

#include "log_once.h"
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

using Frasy::Analyzers::ResultAnalysisResults;
using Frasy::Analyzers::ResultAnalyzer;
using Frasy::Analyzers::ResultCounts;
using Frasy::Report::Result;
namespace fs = std::filesystem;

namespace {
ResultCounts makeCounts(size_t pass, size_t fail, size_t skipped, size_t disabled, size_t incomplete)
{
    ResultCounts counts;
    counts.Counts = {pass, fail, skipped, disabled, incomplete};
    return counts;
}
}    // namespace

// =============================================================================
// ResultCounts
// =============================================================================

TEST(ResultCounts, Empty_ZeroRates)
{
    const ResultCounts counts;
    EXPECT_EQ(counts.getTotal(), 0U);
    EXPECT_FLOAT_EQ(counts.getPassRate(), 0.0f);
    EXPECT_FLOAT_EQ(counts.getPercent(Result::Pass), 0.0f);
}

TEST(ResultCounts, PassRate_OnlyCountsVerdicts)
{
    const auto counts = makeCounts(3, 1, 2, 2, 2);
    EXPECT_EQ(counts.getTotal(), 10U);
    EXPECT_FLOAT_EQ(counts.getPassRate(), 75.0f);
    EXPECT_FLOAT_EQ(counts.getPercent(Result::Disabled), 20.0f);
}

TEST(ResultCounts, Add_CountsEachValue)
{
    ResultCounts counts;
    for (const auto result :
         {Result::Pass, Result::Fail, Result::Skipped, Result::Disabled, Result::Incomplete, Result::Incomplete}) {
        counts.add(result);
    }
    EXPECT_EQ(counts.get(Result::Pass), 1U);
    EXPECT_EQ(counts.get(Result::Incomplete), 2U);
    EXPECT_EQ(counts.getTotal(), 6U);
}

// =============================================================================
// ResultAnalyzer::Analyze — reads pass, fail and debug, v1 and v2 alike
// =============================================================================

class ResultAnalyzerTest : public ::testing::Test {
protected:
    static constexpr auto s_title = "FrasyResultAnalyzerTest";
    const fs::path        m_root  = fs::current_path() / "logs" / s_title;

    static void SetUpTestSuite() { initLogOnce(); }

    void SetUp() override { fs::remove_all(m_root); }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(m_root, ec);
    }

    void write(std::string_view folder, std::string_view name, const std::string& content) const
    {
        fs::create_directories(m_root / folder);
        std::ofstream(m_root / folder / name) << content;
    }

    static std::string v2(std::string_view board, std::string_view sequence, std::string_view t1, std::string_view t2)
    {
        return std::format(
          R"({{ "info": {{ "format_version": 2.0, "result": "{}", "uut": 1, "time": {{ "process": 1.0 }} }},
                "sequences": {{ "S": {{ "result": "{}", "time": {{ "process": 1.0 }}, "tests": {{
                    "T1": {{ "result": "{}", "time": {{ "process": 1.0 }}, "expectations": [] }},
                    "T2": {{ "result": "{}", "time": {{ "process": 1.0 }}, "expectations": [
                        {{ "result": "{}", "method": "ToBeTrue", "expected": true, "name": "E", "note": "E" }} ] }} }} }},
                    "extra": {{ "note": "added by onReport" }} }} }})",
          board,
          sequence,
          t1,
          t2,
          t2 == "pass" ? "pass" : "fail");
    }
};

TEST_F(ResultAnalyzerTest, MixedSet_CountedPerResult)
{
    write("pass", "SN1.json", v2("pass", "pass", "pass", "pass"));
    write("fail", "SN2.json", v2("fail", "fail", "pass", "fail"));
    write("debug", "SN3.json", v2("incomplete", "incomplete", "disabled", "fail"));
    write("pass",
          "SN4.json",
          R"({ "info": { "pass": true, "uut": 1, "time": { "process": 1.0 } },
               "sequences": { "S": { "pass": true, "skipped": false, "enabled": true, "time": { "process": 1.0 },
                                     "tests": {
                   "T1": { "pass": false, "skipped": true, "enabled": true, "time": { "process": 0.0 },
                           "expectations": [] },
                   "T2": { "pass": true, "skipped": false, "enabled": true, "time": { "process": 1.0 },
                           "expectations": [ { "pass": true, "method": "ToBeTrue", "expected": true, "name": "E", "note": "E" } ] } } } } })");

    ResultAnalyzer analyzer;
    const auto     results = analyzer.Analyze(s_title);
    EXPECT_EQ(analyzer.Analyzed, 4U);

    const auto& location = results.Locations.at("Total");
    EXPECT_EQ(location.Results.get(Result::Pass), 2U);
    EXPECT_EQ(location.Results.get(Result::Fail), 1U);
    EXPECT_EQ(location.Results.get(Result::Incomplete), 1U);
    EXPECT_EQ(location.Results.getTotal(), 4U);
    EXPECT_EQ(location.Sequences.size(), 1U);

    const auto& sequence = location.Sequences.at("S");
    EXPECT_EQ(sequence.Results.get(Result::Pass), 2U);
    EXPECT_EQ(sequence.Results.get(Result::Fail), 1U);
    EXPECT_EQ(sequence.Results.get(Result::Incomplete), 1U);

    const auto& t1 = sequence.Tests.at("T1");
    EXPECT_EQ(t1.Results.get(Result::Pass), 2U);
    EXPECT_EQ(t1.Results.get(Result::Disabled), 1U);
    EXPECT_EQ(t1.Results.get(Result::Skipped), 1U);
    // Only the boards where T1 ran give it a duration.
    EXPECT_EQ(t1.Durations.size(), 2U);
    EXPECT_DOUBLE_EQ(t1.AverageDuration, 1.0);

    const auto& t2 = sequence.Tests.at("T2");
    EXPECT_EQ(t2.Results.get(Result::Pass), 2U);
    EXPECT_EQ(t2.Results.get(Result::Fail), 2U);

    const auto expectation = t2.Expectations.at("E")->serialize();
    EXPECT_EQ(expectation.at("total"), 4);
    EXPECT_EQ(expectation.at("passed"), 2);
}

TEST_F(ResultAnalyzerTest, UnreadableReport_SkippedOthersCounted)
{
    write("pass", "SN1.json", v2("pass", "pass", "pass", "pass"));
    write("fail", "SN2.json", v2("great", "fail", "pass", "fail"));
    write("fail", "SN3.json", R"({ "info": { "format_version": 99, "result": "fail", "uut": 1 }, "sequences": {} })");

    ResultAnalyzer analyzer;
    const auto     results = analyzer.Analyze(s_title);
    EXPECT_EQ(analyzer.Analyzed, 3U);
    EXPECT_EQ(results.Locations.at("Total").Results.getTotal(), 1U);
}

TEST_F(ResultAnalyzerTest, BadScopeResult_NothingOfTheReportCounted)
{
    write("pass", "SN1.json", v2("pass", "pass", "pass", "pass"));
    write("fail", "SN2.json", v2("fail", "great", "pass", "fail"));
    write("fail", "SN3.json", v2("fail", "fail", "pass", "great"));

    ResultAnalyzer analyzer;
    const auto     results  = analyzer.Analyze(s_title);
    const auto&    location = results.Locations.at("Total");
    EXPECT_EQ(location.Results.getTotal(), 1U);
    EXPECT_EQ(location.Durations.size(), 1U);
    const auto& sequence = location.Sequences.at("S");
    EXPECT_EQ(sequence.Results.getTotal(), 1U);
    EXPECT_EQ(sequence.Tests.at("T1").Results.getTotal(), 1U);
}

TEST_F(ResultAnalyzerTest, BadExpectation_OnlyThatExpectationDropped)
{
    write("fail",
          "SN1.json",
          R"({ "info": { "format_version": 2, "result": "fail", "uut": 1, "time": { "process": 1.0 } },
               "sequences": { "S": { "result": "fail", "time": { "process": 1.0 }, "tests": {
                   "T": { "result": "fail", "time": { "process": 1.0 }, "expectations": [
                       { "method": "ToBeInRange", "min": 0, "max": 1, "value": 0.5, "name": "NoResult", "note": "n" },
                       { "result": "fail", "method": "ToBeInRange", "min": 0, "max": 1, "name": "NoValue",
                         "note": "n" },
                       { "result": "pass", "method": "ToBeInRange", "min": 0, "max": 1, "value": 0.5, "name": "Ok",
                         "note": "n" } ] } } } } })");

    ResultAnalyzer analyzer;
    const auto     results = analyzer.Analyze(s_title);
    const auto&    test    = results.Locations.at("Total").Sequences.at("S").Tests.at("T");
    EXPECT_EQ(test.Results.get(Result::Fail), 1U);
    EXPECT_FALSE(test.Expectations.contains("NoResult"));
    // Created before its value was found missing: nothing counted, and no stats made from no values.
    EXPECT_EQ(test.Expectations.at("NoValue")->serialize().at("total"), 0);
    EXPECT_EQ(test.Expectations.at("Ok")->serialize().at("passed"), 1);
}

TEST_F(ResultAnalyzerTest, TestThatNeverRan_NoDuration)
{
    // A sequence whose body raised: its test failed without starting.
    write("fail",
          "SN1.json",
          R"({ "info": { "format_version": 2.0, "result": "fail", "uut": 1, "time": { "process": 1.0 } },
               "sequences": { "S": { "result": "fail", "time": { "process": 1.0 }, "tests": {
                   "T": { "result": "fail", "time": { "process": 0.0, "stop": 0.0 }, "expectations": [] } } } } })");
    write("pass",
          "SN2.json",
          R"({ "info": { "format_version": 2.0, "result": "pass", "uut": 1, "time": { "process": 1.0 } },
               "sequences": { "S": { "result": "pass", "time": { "process": 1.0 }, "tests": {
                   "T": { "result": "pass", "time": { "process": 2.0, "stop": 3.0 }, "expectations": [] } } } } })");

    ResultAnalyzer analyzer;
    const auto     results = analyzer.Analyze(s_title);
    const auto&    test    = results.Locations.at("Total").Sequences.at("S").Tests.at("T");
    EXPECT_EQ(test.Results.get(Result::Fail), 1U);
    EXPECT_EQ(test.Durations.size(), 1U);
    EXPECT_DOUBLE_EQ(test.AverageDuration, 2.0);
}

TEST_F(ResultAnalyzerTest, V1SkippedAndDisabledSequences_TestsTakeTheirResult)
{
    write("pass",
          "SN1.json",
          R"({ "info": { "pass": true, "uut": 1, "time": { "process": 1.0 } }, "sequences": {
               "Skipped": { "pass": false, "skipped": true, "enabled": true, "time": { "process": 0.0 }, "tests": {
                   "T": { "pass": false, "skipped": false, "time": { "process": 0.0 }, "expectations": [] } } },
               "Disabled": { "pass": false, "skipped": true, "enabled": false, "time": { "process": 0.0 }, "tests": {
                   "T": { "pass": false, "skipped": false, "time": { "process": 0.0 }, "expectations": [] } } } } })");

    ResultAnalyzer analyzer;
    const auto     results   = analyzer.Analyze(s_title);
    const auto&    sequences = results.Locations.at("Total").Sequences;
    EXPECT_EQ(sequences.at("Skipped").Results.get(Result::Skipped), 1U);
    EXPECT_EQ(sequences.at("Skipped").Tests.at("T").Results.get(Result::Skipped), 1U);
    EXPECT_EQ(sequences.at("Disabled").Results.get(Result::Disabled), 1U);
    EXPECT_EQ(sequences.at("Disabled").Tests.at("T").Results.get(Result::Disabled), 1U);
    EXPECT_TRUE(sequences.at("Skipped").Tests.at("T").Durations.empty());
    EXPECT_TRUE(sequences.at("Disabled").Tests.at("T").Durations.empty());
}

// =============================================================================
// Saved analyses
// =============================================================================

class SavedAnalysisTest : public ::testing::Test {
protected:
    fs::path m_file;

    void SetUp() override
    {
        const auto* test = ::testing::UnitTest::GetInstance()->current_test_info();
        const auto  dir  = fs::temp_directory_path() / "frasy_tests" / test->test_suite_name() / test->name();
        fs::remove_all(dir);
        fs::create_directories(dir);
        m_file = dir / "analysis.json";
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(m_file.parent_path(), ec);
    }
};

TEST_F(SavedAnalysisTest, RoundTrip_KeepsCounts)
{
    ResultAnalysisResults results;
    auto&                 location = results.Locations["Total"];
    location.Name                  = "Total";
    location.Results               = makeCounts(5, 4, 0, 0, 3);
    location.Durations             = {1.0};
    auto& sequence                 = location.Sequences["S"];
    sequence.Name                  = "S";
    sequence.Results               = makeCounts(1, 2, 3, 4, 5);
    sequence.Tests["T"].Name       = "T";
    sequence.Tests["T"].Results    = makeCounts(6, 7, 8, 9, 0);

    Frasy::Analyzers::Save(results, m_file.string());
    const auto loaded = Frasy::Analyzers::Load(m_file.string());

    const auto& loadedLocation = loaded.Locations.at("Total");
    EXPECT_EQ(loadedLocation.Results.Counts, location.Results.Counts);
    EXPECT_EQ(loadedLocation.Sequences.at("S").Results.Counts, sequence.Results.Counts);
    EXPECT_EQ(loadedLocation.Sequences.at("S").Tests.at("T").Results.Counts, sequence.Tests["T"].Results.Counts);
}

TEST_F(SavedAnalysisTest, LegacyCounts_Converted)
{
    // Saved before per-result counts: disabled scopes were counted as skipped too.
    std::ofstream(m_file) << R"({ "Total": { "name": "Total", "version": "", "passed": 7, "passed_percent": 70.0,
        "total": 10, "durations": [], "average_duration": 0.0, "sequences": { "S": {
            "name": "S", "passed": 4, "passed_percent": 66.0, "enabled": 8, "enabled_percent": 80.0, "skipped": 4,
            "skipped_percent": 40.0, "total": 10, "durations": [], "average_duration": 0.0, "tests": {} } } } })";

    const auto loaded = Frasy::Analyzers::Load(m_file.string());

    const auto& location = loaded.Locations.at("Total");
    EXPECT_EQ(location.Results.get(Result::Pass), 7U);
    EXPECT_EQ(location.Results.get(Result::Fail), 3U);
    const auto& sequence = location.Sequences.at("S");
    EXPECT_EQ(sequence.Results.get(Result::Pass), 4U);
    EXPECT_EQ(sequence.Results.get(Result::Disabled), 2U);
    EXPECT_EQ(sequence.Results.get(Result::Skipped), 2U);
    EXPECT_EQ(sequence.Results.get(Result::Fail), 2U);
    EXPECT_EQ(sequence.Results.getTotal(), 10U);
}

TEST_F(SavedAnalysisTest, LegacyCounts_InconsistentSaturateAtZero)
{
    // Sequence A: fewer skipped than disabled. Sequence B: more counted than its total.
    std::ofstream(m_file) << R"({ "Total": { "name": "Total", "version": "", "passed": 1, "total": 1,
        "durations": [], "average_duration": 0.0, "sequences": {
            "A": { "name": "A", "passed": 2, "enabled": 2, "skipped": 1, "total": 5, "durations": [],
                   "average_duration": 0.0, "tests": {} },
            "B": { "name": "B", "passed": 4, "enabled": 4, "skipped": 3, "total": 5, "durations": [],
                   "average_duration": 0.0, "tests": {} } } } })";

    const auto  loaded    = Frasy::Analyzers::Load(m_file.string());
    const auto& location  = loaded.Locations.at("Total");
    const auto& sequences = location.Sequences;
    EXPECT_EQ(location.Results.get(Result::Pass), 1U);
    EXPECT_EQ(location.Results.get(Result::Disabled), 0U);
    EXPECT_EQ(location.Results.get(Result::Skipped), 0U);
    EXPECT_EQ(sequences.at("A").Results.Counts, makeCounts(2, 0, 0, 3, 0).Counts);
    EXPECT_EQ(sequences.at("B").Results.Counts, makeCounts(4, 0, 2, 1, 0).Counts);
}

TEST_F(SavedAnalysisTest, MissingResultKeys_Zero)
{
    std::ofstream(m_file) << R"({ "Total": { "name": "Total", "version": "", "results": { "pass": 3 },
        "durations": [], "average_duration": 0.0, "sequences": {} } })";

    const auto loaded = Frasy::Analyzers::Load(m_file.string());
    EXPECT_EQ(loaded.Locations.at("Total").Results.Counts, makeCounts(3, 0, 0, 0, 0).Counts);
}

TEST_F(SavedAnalysisTest, ResultsNotAnObject_Throws)
{
    std::ofstream(m_file) << R"({ "Total": { "name": "Total", "version": "", "results": [ 1, 2 ],
        "durations": [], "average_duration": 0.0, "sequences": {} } })";

    EXPECT_ANY_THROW((void)Frasy::Analyzers::Load(m_file.string()));
}
