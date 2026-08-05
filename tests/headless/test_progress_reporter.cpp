/**
 * @file    test_progress_reporter.cpp
 * @brief   Unit tests for progress reporter event formatting.
 */
#include <gtest/gtest.h>
#include <utils/headless/progress_reporter.h>
#include <json.hpp>
#include <sstream>

using namespace Frasy::Headless;
using json = nlohmann::json;

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

    ProgressEvent event {ProgressEvent::SequenceStart, 1, "Power On", "", "", true};
    auto          output = captureEvent(reporter, event);

    EXPECT_NE(output.find(">> Power On"), std::string::npos);
    EXPECT_NE(output.find("[UUT1]"), std::string::npos);
}

TEST(ProgressReporter, HumanSequenceEndPass)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("human", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::SequenceEnd, 1, "Power On", "", "", true};
    auto          output = captureEvent(reporter, event);

    EXPECT_NE(output.find("[PASS]"), std::string::npos);
    EXPECT_NE(output.find("<< Power On"), std::string::npos);
}

TEST(ProgressReporter, HumanSequenceEndFail)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("human", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::SequenceEnd, 1, "Power On", "", "", false};
    auto          output = captureEvent(reporter, event);

    EXPECT_NE(output.find("[FAIL]"), std::string::npos);
}

TEST(ProgressReporter, HumanTestStart)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("human", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::TestStart, 1, "Check Voltage", "Power On", "", true};
    auto          output = captureEvent(reporter, event);

    EXPECT_NE(output.find("> Power On > Check Voltage"), std::string::npos);
}

TEST(ProgressReporter, HumanExpectationPass)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("human", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::Expectation, 1, "Supply Voltage", "Power On", "Check Voltage", true};
    auto          output = captureEvent(reporter, event);

    EXPECT_NE(output.find("[PASS]"), std::string::npos);
    EXPECT_NE(output.find("Supply Voltage"), std::string::npos);
}

// --- JSON format tests ---

TEST(ProgressReporter, JsonSequenceStart)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("json", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::SequenceStart, 1, "Power On", "", "", true};
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

    ProgressEvent event {ProgressEvent::TestEnd, 1, "Check Voltage", "Power On", "", false};
    auto          output = captureEvent(reporter, event);
    auto          j      = json::parse(output);

    EXPECT_EQ(j["type"], "test_end");
    EXPECT_EQ(j["test"], "Check Voltage");
    EXPECT_EQ(j["sequence"], "Power On");
    EXPECT_EQ(j["pass"], false);
}

TEST(ProgressReporter, JsonExpectation)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("json", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::Expectation, 1, "Supply Voltage", "Power On", "Check Voltage", true};
    auto          output = captureEvent(reporter, event);
    auto          j      = json::parse(output);

    EXPECT_EQ(j["type"], "expectation");
    EXPECT_EQ(j["name"], "Supply Voltage");
    EXPECT_EQ(j["sequence"], "Power On");
    EXPECT_EQ(j["test"], "Check Voltage");
    EXPECT_EQ(j["pass"], true);
}

TEST(ProgressReporter, JsonHasTimestamp)
{
    std::mutex       ioMutex;
    ProgressReporter reporter("json", "TestProduct", {"SN001"}, ioMutex);

    ProgressEvent event {ProgressEvent::SequenceStart, 1, "Basic", "", "", true};
    auto          output = captureEvent(reporter, event);
    auto          j      = json::parse(output);

    EXPECT_TRUE(j.contains("timestamp"));
    EXPECT_FALSE(j["timestamp"].get<std::string>().empty());
}
