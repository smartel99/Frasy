/**
 * @file    test_uut_results.cpp
 * @brief   Unit tests for the MCP get_results output.
 */
#include "report/log_once.h"
#include "utils/mcp/uut_results.h"

#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <fstream>

using Frasy::UutState;
using Frasy::Mcp::makeRunResults;
using Frasy::Mcp::makeUutResult;
using json   = nlohmann::json;
namespace fs = std::filesystem;

class UutResultsTest : public ::testing::Test {
protected:
    fs::path m_report;

    static void SetUpTestSuite() { initLogOnce(); }

    void SetUp() override
    {
        const auto* test = ::testing::UnitTest::GetInstance()->current_test_info();
        const auto  dir  = fs::temp_directory_path() / "frasy_tests" / test->test_suite_name() / test->name();
        fs::remove_all(dir);
        fs::create_directories(dir);
        m_report = dir / "1.json";
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(m_report.parent_path(), ec);
    }

    void write(const std::string& content) const { std::ofstream(m_report) << content; }
};

TEST_F(UutResultsTest, ReadableReport_CountsAndResult)
{
    write(R"({ "info": { "format_version": 2, "result": "incomplete", "time": { "elapsed": 1.5 } }, "sequences": {
        "S": { "result": "incomplete", "tests": { "A": { "result": "pass" }, "B": { "result": "disabled" } } } } })");
    const auto result = makeUutResult(1, "SN1", UutState::IncompletePassed, m_report);
    EXPECT_EQ(result["uut"], 1);
    EXPECT_EQ(result["serial"], "SN1");
    EXPECT_EQ(result["state"], "incomplete_passed");
    EXPECT_EQ(result["result"], "incomplete");
    EXPECT_EQ(result["tests_passed"], 1);
    EXPECT_EQ(result["tests_total"], 2);
    EXPECT_EQ(result["test_results"]["disabled"], 1);
    EXPECT_DOUBLE_EQ(result["duration"].get<double>(), 1.5);
    EXPECT_EQ(result["report_path"], m_report.generic_string());
    EXPECT_EQ(result["pass"], false);
}

TEST_F(UutResultsTest, UnreadableReport_ZeroCounts)
{
    write(R"({ "info": { "format_version": 2, "result": "fail" }, "sequences": {
        "S": { "result": "fail", "tests": { "A": { "result": "great" } } } } })");
    const auto result = makeUutResult(1, "SN1", UutState::Failed, m_report);
    EXPECT_EQ(result["result"], "fail");
    EXPECT_EQ(result["tests_passed"], 0);
    EXPECT_EQ(result["tests_total"], 0);
    EXPECT_EQ(result["test_results"]["fail"], 0);
}

TEST_F(UutResultsTest, NoReport_ZeroCountsNoPath)
{
    const auto result = makeUutResult(2, "", UutState::Disabled, m_report);
    EXPECT_EQ(result["result"], "disabled");
    EXPECT_EQ(result["tests_total"], 0);
    EXPECT_FALSE(result.contains("report_path"));
    EXPECT_FALSE(result.contains("duration"));
}

TEST_F(UutResultsTest, Error_NoResult)
{
    EXPECT_FALSE(makeUutResult(1, "SN1", UutState::Error, m_report).contains("result"));
}

TEST(RunResults, OverallPass_OnlyWhenEveryUutPassed)
{
    const std::array passed  = {UutState::Passed, UutState::Passed};
    const auto       results = makeRunResults("P", passed, json::array({json::object()}));
    EXPECT_EQ(results["product"], "P");
    EXPECT_EQ(results["overall_pass"], true);
    EXPECT_EQ(results["uuts"].size(), 1U);

    const std::array incomplete = {UutState::Passed, UutState::IncompletePassed};
    EXPECT_EQ(makeRunResults("P", incomplete, json::array())["overall_pass"], false);
}
