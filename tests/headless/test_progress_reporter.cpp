/**
 * @file    test_progress_reporter.cpp
 * @brief   Unit tests for progress reporter event formatting.
 */
#include <gtest/gtest.h>
#include <json.hpp>
#include <sstream>
#include <utils/headless/progress_reporter.h>

using namespace Frasy::Headless;
using json   = nlohmann::json;
using Result = Frasy::Report::Result;

namespace {
/// Capture stdout output from onEvent
std::string captureEvent(ProgressReporter& reporter, const ProgressEvent& event)
{
    std::stringstream buffer;
    auto              oldBuf = std::cout.rdbuf(buffer.rdbuf());
    reporter.onEvent(event);
    std::cout.rdbuf(oldBuf);
    return buffer.str();
}
}    // namespace

// --- Human format tests ---

TEST(ProgressReporter, HumanSequenceStart)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("human", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::SequenceStart, 1, "Power On", "", "", Result::Pass};
    auto          output = captureEvent(reporter, event);

    EXPECT_NE(output.find(">> Power On"), std::string::npos);
    EXPECT_NE(output.find("[UUT1]"), std::string::npos);
}

TEST(ProgressReporter, HumanSequenceEndPass)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("human", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::SequenceEnd, 1, "Power On", "", "", Result::Pass};
    auto          output = captureEvent(reporter, event);

    EXPECT_NE(output.find("[PASS]"), std::string::npos);
    EXPECT_NE(output.find("<< Power On"), std::string::npos);
}

TEST(ProgressReporter, HumanSequenceEndFail)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("human", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::SequenceEnd, 1, "Power On", "", "", Result::Fail};
    auto          output = captureEvent(reporter, event);

    EXPECT_NE(output.find("[FAIL]"), std::string::npos);
}

TEST(ProgressReporter, HumanTestStart)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("human", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::TestStart, 1, "Check Voltage", "Power On", "", Result::Pass};
    auto          output = captureEvent(reporter, event);

    EXPECT_NE(output.find("> Power On > Check Voltage"), std::string::npos);
}

TEST(ProgressReporter, HumanExpectationPass)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("human", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::Expectation, 1, "Supply Voltage", "Power On", "Check Voltage", Result::Pass};
    auto          output = captureEvent(reporter, event);

    EXPECT_NE(output.find("[PASS]"), std::string::npos);
    EXPECT_NE(output.find("Supply Voltage"), std::string::npos);
}

// --- JSON format tests ---

TEST(ProgressReporter, JsonSequenceStart)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("json", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::SequenceStart, 1, "Power On", "", "", Result::Pass};
    auto          output = captureEvent(reporter, event);
    auto          j      = json::parse(output);

    EXPECT_EQ(j["type"], "sequence_start");
    EXPECT_EQ(j["sequence"], "Power On");
    EXPECT_EQ(j["uut"], 1);
    EXPECT_EQ(j["serial"], "SN001");
}

TEST(ProgressReporter, JsonTestEnd)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("json", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::TestEnd, 1, "Check Voltage", "Power On", "", Result::Fail};
    auto          output = captureEvent(reporter, event);
    auto          j      = json::parse(output);

    EXPECT_EQ(j["type"], "test_end");
    EXPECT_EQ(j["test"], "Check Voltage");
    EXPECT_EQ(j["sequence"], "Power On");
    EXPECT_EQ(j["result"], "fail");
    EXPECT_EQ(j["pass"], false);
}

TEST(ProgressReporter, JsonExpectation)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("json", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::Expectation, 1, "Supply Voltage", "Power On", "Check Voltage", Result::Pass};
    auto          output = captureEvent(reporter, event);
    auto          j      = json::parse(output);

    EXPECT_EQ(j["type"], "expectation");
    EXPECT_EQ(j["name"], "Supply Voltage");
    EXPECT_EQ(j["sequence"], "Power On");
    EXPECT_EQ(j["test"], "Check Voltage");
    EXPECT_EQ(j["result"], "pass");
    EXPECT_EQ(j["pass"], true);
}

TEST(ProgressReporter, JsonHasTimestamp)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("json", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::SequenceStart, 1, "Basic", "", "", Result::Pass};
    auto          output = captureEvent(reporter, event);
    auto          j      = json::parse(output);

    EXPECT_TRUE(j.contains("timestamp"));
    EXPECT_FALSE(j["timestamp"].get<std::string>().empty());
}

// --- Results other than pass and fail ---

TEST(ProgressReporter, JsonEndEvents_HaveResult)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("json", "TestProduct", {"SN001"}, ioMutex);

    for (const auto type : {ProgressEvent::SequenceEnd, ProgressEvent::TestEnd, ProgressEvent::Expectation}) {
        ProgressEvent event {type, 1, "N", "S", "T", Result::Disabled};
        auto          j = json::parse(captureEvent(reporter, event));
        EXPECT_EQ(j["result"], "disabled") << j.dump();
        EXPECT_EQ(j["pass"], false) << j.dump();
    }
}

TEST(ProgressReporter, JsonStartEvents_NoResult)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("json", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::TestStart, 1, "T", "S", "", Result::Pass};
    auto          j = json::parse(captureEvent(reporter, event));
    EXPECT_FALSE(j.contains("result"));
    EXPECT_FALSE(j.contains("pass"));
}

TEST(ProgressReporter, HumanTags_PerResult)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("human", "TestProduct", {"SN001"}, ioMutex);

    const std::pair<Result, std::string_view> cases[] = {
      {Result::Skipped, "[SKIPPED]"},
      {Result::Disabled, "[DISABLED]"},
      {Result::Incomplete, "[INCOMPLETE]"},
    };
    for (const auto& [result, tag] : cases) {
        ProgressEvent event {ProgressEvent::SequenceEnd, 1, "S", "", "", result};
        const auto    output = captureEvent(reporter, event);
        EXPECT_NE(output.find(tag), std::string::npos) << output;
        EXPECT_EQ(output.find("[PASS]"), std::string::npos) << output;
        // Yellow, neither green nor red.
        EXPECT_NE(output.find("\033[33m"), std::string::npos) << output;
    }
}
