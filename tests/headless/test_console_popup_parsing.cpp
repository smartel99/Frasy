/**
 * @file    test_console_popup_parsing.cpp
 * @brief   Unit tests for console popup handler line parsing.
 */
#include <gtest/gtest.h>
#include <utils/headless/console_popup_handler.h>

using namespace Frasy::Headless;

// --- parseHumanLine tests ---

TEST(ConsolePopupParsing, HumanLineButtonOnly)
{
    auto resp = parseHumanLine("OK");
    EXPECT_EQ(resp.button, "OK");
    EXPECT_TRUE(resp.inputs.empty());
}

TEST(ConsolePopupParsing, HumanLineButtonCasePreserved)
{
    auto resp = parseHumanLine("Submit");
    EXPECT_EQ(resp.button, "Submit");
}

TEST(ConsolePopupParsing, HumanLineSingleInput)
{
    auto resp = parseHumanLine("1=hello");
    EXPECT_TRUE(resp.button.empty());
    ASSERT_EQ(resp.inputs.size(), 1);
    EXPECT_EQ(resp.inputs[1], "hello");
}

TEST(ConsolePopupParsing, HumanLineInputWithEquals)
{
    // Value itself contains = sign
    auto resp = parseHumanLine("1=a=b=c");
    EXPECT_TRUE(resp.button.empty());
    EXPECT_EQ(resp.inputs[1], "a=b=c");
}

TEST(ConsolePopupParsing, HumanLineInputIndex2)
{
    auto resp = parseHumanLine("2=world");
    EXPECT_TRUE(resp.button.empty());
    EXPECT_EQ(resp.inputs[2], "world");
}

TEST(ConsolePopupParsing, HumanLineNonNumericEquals)
{
    // "name=value" — "name" is not a number, so treat as button
    auto resp = parseHumanLine("name=value");
    EXPECT_EQ(resp.button, "name=value");
    EXPECT_TRUE(resp.inputs.empty());
}

TEST(ConsolePopupParsing, HumanLineEmptyValue)
{
    auto resp = parseHumanLine("1=");
    EXPECT_TRUE(resp.button.empty());
    EXPECT_EQ(resp.inputs[1], "");
}

TEST(ConsolePopupParsing, HumanLineQuestionMark)
{
    // ? is not an input or a button match — it's handled separately by the caller
    // But parseHumanLine just returns it as a button
    auto resp = parseHumanLine("?");
    EXPECT_EQ(resp.button, "?");
}

// --- parseJsonLine tests ---

TEST(ConsolePopupParsing, JsonLineButtonOnly)
{
    auto resp = parseJsonLine(R"({"button":"OK"})");
    EXPECT_EQ(resp.button, "OK");
    EXPECT_TRUE(resp.inputs.empty());
}

TEST(ConsolePopupParsing, JsonLineButtonAndInputs)
{
    auto resp = parseJsonLine(R"({"button":"Submit","inputs":{"1":"hello","2":"world"}})");
    EXPECT_EQ(resp.button, "Submit");
    ASSERT_EQ(resp.inputs.size(), 2);
    EXPECT_EQ(resp.inputs[1], "hello");
    EXPECT_EQ(resp.inputs[2], "world");
}

TEST(ConsolePopupParsing, JsonLineInputsOnly)
{
    auto resp = parseJsonLine(R"({"inputs":{"1":"value"}})");
    EXPECT_TRUE(resp.button.empty());
    EXPECT_EQ(resp.inputs[1], "value");
}

TEST(ConsolePopupParsing, JsonLineInvalidJson)
{
    auto resp = parseJsonLine("not json at all");
    EXPECT_TRUE(resp.button.empty());
    EXPECT_TRUE(resp.inputs.empty());
}

TEST(ConsolePopupParsing, JsonLineEmptyObject)
{
    auto resp = parseJsonLine("{}");
    EXPECT_TRUE(resp.button.empty());
    EXPECT_TRUE(resp.inputs.empty());
}

TEST(ConsolePopupParsing, JsonLineMultipleInputs)
{
    auto resp = parseJsonLine(R"({"inputs":{"1":"LOT-001","2":"OP-42","3":"BATCH-X"},"button":"Confirm"})");
    EXPECT_EQ(resp.button, "Confirm");
    ASSERT_EQ(resp.inputs.size(), 3);
    EXPECT_EQ(resp.inputs[1], "LOT-001");
    EXPECT_EQ(resp.inputs[2], "OP-42");
    EXPECT_EQ(resp.inputs[3], "BATCH-X");
}
