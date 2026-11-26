#include "layers/result_viewer.h"
#include "utils/report/result.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

using Frasy::ResultViewer;
using Frasy::Report::Result;
namespace fs = std::filesystem;

// =============================================================================
// ResultViewer::LoadResults — v1 and v2 reports
// =============================================================================

class ResultViewerLoadTest : public ::testing::Test {
protected:
    fs::path m_file;

    void SetUp() override
    {
        const auto* test = ::testing::UnitTest::GetInstance()->current_test_info();
        const auto  dir  = fs::temp_directory_path() / "frasy_tests" / test->test_suite_name() / test->name();
        fs::remove_all(dir);
        fs::create_directories(dir);
        m_file = dir / "1.json";
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(m_file.parent_path(), ec);
    }

    ResultViewer::OverallTestResult load(const std::string& sequences, const std::string& info)
    {
        std::ofstream(m_file) << R"({ "info": { "serial": "SN1", "date": "today", "uut": 1, "time": { "process": 1.0 },
                                                "version": { "scripts": "1.0" }, )"
                              << info << R"( }, "sequences": )" << sequences << "}";
        return ResultViewer::LoadResults(m_file.string());
    }
};

TEST_F(ResultViewerLoadTest, V2_ResultsRead)
{
    const auto log = load(R"({ "S": { "result": "incomplete", "time": { "process": 1.0 }, "tests": {
                                 "T1": { "result": "disabled", "time": { "process": 0.0 }, "expectations": [] },
                                 "T2": { "result": "fail", "time": { "process": 1.0 }, "expectations": [
                                     { "result": "fail", "note": "n" }, { "result": "pass" } ] } } } })",
                          R"("format_version": 2.0, "result": "incomplete")");
    EXPECT_EQ(log.Result, Result::Incomplete);
    EXPECT_TRUE(log.HasFailure);
    const auto& sequence = log.Sequences.at("S");
    EXPECT_EQ(sequence.Result, Result::Incomplete);
    EXPECT_EQ(sequence.Tests.at("T1").Result, Result::Disabled);
    const auto& test = sequence.Tests.at("T2");
    EXPECT_EQ(test.Result, Result::Fail);
    ASSERT_EQ(test.Expectations.size(), 2U);
    EXPECT_EQ(test.Expectations[0].Result, Result::Fail);
    EXPECT_EQ(test.Expectations[0].Details.at("note"), "n");
    EXPECT_EQ(test.Expectations[1].Result, Result::Pass);
}

TEST_F(ResultViewerLoadTest, V2_IncompleteWithoutFailure)
{
    const auto log = load(R"({ "S": { "result": "disabled", "time": { "process": 0.0 }, "tests": {} } })",
                          R"("format_version": 2, "result": "incomplete")");
    EXPECT_EQ(log.Result, Result::Incomplete);
    EXPECT_FALSE(log.HasFailure);
}

TEST_F(ResultViewerLoadTest, V1_BooleansRead)
{
    const auto log = load(R"({ "S": { "pass": false, "skipped": false, "enabled": true, "time": { "process": 1.0 },
                                      "tests": {
                                 "T1": { "pass": true, "skipped": false, "enabled": true, "time": { "process": 1.0 },
                                         "expectations": [ { "pass": true } ] },
                                 "T2": { "pass": false, "skipped": false, "enabled": true, "time": { "process": 1.0 },
                                         "expectations": [ { "pass": false } ] },
                                 "T3": { "pass": false, "skipped": true, "enabled": false, "time": { "process": 0.0 },
                                         "expectations": [] } } } })",
                          R"("pass": false)");
    // A disabled test makes the board incomplete, like the orchestrator decides it.
    EXPECT_EQ(log.Result, Result::Incomplete);
    EXPECT_TRUE(log.HasFailure);
    EXPECT_EQ(log.Sequences.at("S").Result, Result::Fail);
    const auto& tests = log.Sequences.at("S").Tests;
    EXPECT_EQ(tests.at("T1").Result, Result::Pass);
    EXPECT_EQ(tests.at("T1").Expectations.at(0).Result, Result::Pass);
    EXPECT_EQ(tests.at("T2").Result, Result::Fail);
    EXPECT_EQ(tests.at("T2").Expectations.at(0).Result, Result::Fail);
    EXPECT_EQ(tests.at("T3").Result, Result::Disabled);
}

TEST_F(ResultViewerLoadTest, V1_TestsOfSkippedSequenceAreSkipped)
{
    // v1 left the tests of a skipped sequence untouched.
    const auto log = load(R"({ "S": { "pass": false, "skipped": true, "enabled": true, "time": { "process": 0.0 },
                                      "tests": { "T": { "pass": false, "skipped": false, "enabled": true,
                                                        "time": { "process": 0.0 }, "expectations": [] } } } })",
                          R"("pass": true)");
    EXPECT_EQ(log.Result, Result::Pass);
    EXPECT_EQ(log.Sequences.at("S").Result, Result::Skipped);
    EXPECT_EQ(log.Sequences.at("S").Tests.at("T").Result, Result::Skipped);
}

TEST_F(ResultViewerLoadTest, EntriesWithoutResult_Ignored)
{
    const auto log = load(R"({ "S": { "result": "pass", "time": { "process": 1.0 }, "tests": {
                                 "T": { "result": "pass", "time": { "process": 1.0 }, "expectations": [] },
                                 "extra": { "note": "added by onReport" } } },
                               "extra": { "note": "added by onReport" } })",
                          R"("format_version": 2, "result": "pass")");
    EXPECT_EQ(log.Sequences.size(), 1U);
    EXPECT_EQ(log.Sequences.at("S").Tests.size(), 1U);
}

TEST_F(ResultViewerLoadTest, UnknownResult_Throws)
{
    EXPECT_THROW(load(R"({ "S": { "result": "great", "time": { "process": 1.0 }, "tests": {} } })",
                      R"("format_version": 2, "result": "pass")"),
                 Frasy::Report::ReportFormatError);
}

TEST_F(ResultViewerLoadTest, NewerVersion_Throws)
{
    EXPECT_THROW(load("{}", R"("format_version": 3, "result": "pass")"), Frasy::Report::ReportFormatError);
}

TEST_F(ResultViewerLoadTest, V1_TestsOfDisabledSequenceAreDisabled)
{
    const auto log = load(R"({ "S": { "pass": false, "skipped": true, "enabled": false, "time": { "process": 0.0 },
                                      "tests": { "T": { "pass": false, "skipped": false, "time": { "process": 0.0 },
                                                        "expectations": [] } } } })",
                          R"("pass": true)");
    EXPECT_EQ(log.Sequences.at("S").Result, Result::Disabled);
    EXPECT_EQ(log.Sequences.at("S").Tests.at("T").Result, Result::Disabled);
}

TEST_F(ResultViewerLoadTest, ExpectationWithoutResult_ShownWithoutResult)
{
    const auto  log          = load(R"({ "S": { "result": "pass", "time": { "process": 1.0 }, "tests": {
                                 "T": { "result": "pass", "time": { "process": 1.0 }, "expectations": [
                                     { "note": "added by onReport" }, { "result": "pass" } ] } } } })",
                          R"("format_version": 2, "result": "pass")");
    const auto& expectations = log.Sequences.at("S").Tests.at("T").Expectations;
    ASSERT_EQ(expectations.size(), 2U);
    EXPECT_FALSE(expectations[0].Result.has_value());
    EXPECT_EQ(expectations[0].Details.at("note"), "added by onReport");
    EXPECT_EQ(expectations[1].Result, Result::Pass);
}

TEST_F(ResultViewerLoadTest, V1_ExpectationWithoutPass_ShownWithoutResult)
{
    const auto log = load(R"({ "S": { "pass": true, "skipped": false, "enabled": true, "time": { "process": 1.0 },
                                      "tests": { "T": { "pass": true, "skipped": false, "enabled": true,
                                                        "time": { "process": 1.0 },
                                                        "expectations": [ { "note": "n" } ] } } } })",
                          R"("pass": true)");
    EXPECT_FALSE(log.Sequences.at("S").Tests.at("T").Expectations.at(0).Result.has_value());
}

// =============================================================================
// ResultViewer display rules
// =============================================================================

TEST(ResultViewerDisplay, OpenByDefault_FailedAndIncompleteOnly)
{
    EXPECT_TRUE(ResultViewer::isOpenByDefault(Result::Fail));
    EXPECT_TRUE(ResultViewer::isOpenByDefault(Result::Incomplete));
    EXPECT_FALSE(ResultViewer::isOpenByDefault(Result::Pass));
    EXPECT_FALSE(ResultViewer::isOpenByDefault(Result::Skipped));
    EXPECT_FALSE(ResultViewer::isOpenByDefault(Result::Disabled));
    EXPECT_FALSE(ResultViewer::isOpenByDefault(std::nullopt));
}

TEST(ResultViewerDisplay, TabColor)
{
    EXPECT_EQ(ResultViewer::getTabColor(Result::Pass, false), std::nullopt);
    EXPECT_EQ(ResultViewer::getTabColor(Result::Fail, true), ResultViewer::s_failColor);
    EXPECT_EQ(ResultViewer::getTabColor(Result::Incomplete, false), ResultViewer::s_incompleteColor);
    EXPECT_EQ(ResultViewer::getTabColor(Result::Incomplete, true), ResultViewer::s_failColor);
}
