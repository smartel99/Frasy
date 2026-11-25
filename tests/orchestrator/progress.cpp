#include "orchestrator/execution_fixture.h"
#include "report/log_once.h"
#include "utils/lua/orchestrator/orchestrator.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

// =============================================================================
// __progress.report — every event carries a result string
// =============================================================================

class ProgressTest : public ExecutionTest {
protected:
    void SetUp() override
    {
        ExecutionTest::SetUp();
        lua.script(R"(
            events = {}
            __progress.report = function(kind, uut, name, parentA, parentB, result)
                table.insert(events, kind .. ':' .. name .. ':' .. tostring(result))
            end
        )");
    }

    std::vector<std::string> events() { return lua.script("return events").get<std::vector<std::string>>(); }

    bool hasEvent(const std::string& event)
    {
        const auto all = events();
        return std::ranges::find(all, event) != all.end();
    }
};

TEST_F(ProgressTest, TestEnd_CarriesEveryResult)
{
    lua.script(R"(Define({ { name = 'S', tests = {
        { name = 'Off' },
        { name = 'Skip', func = function() error(UnmetRequirement('no')) end },
        { name = 'Bad', func = Fail },
        { name = 'Ok' },
    } } }))");
    lua.script(
      "Context.orchestrator.enable_list = { S = { enabled = true, Off = { enabled = false }, Skip = { enabled = true "
      "}, "
      "Bad = { enabled = true }, Ok = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_TRUE(hasEvent("test_end:Off:disabled"));
    EXPECT_TRUE(hasEvent("test_end:Skip:skipped"));
    EXPECT_TRUE(hasEvent("test_end:Bad:fail"));
    EXPECT_TRUE(hasEvent("test_end:Ok:pass"));
    EXPECT_TRUE(hasEvent("expectation:E:fail"));
    EXPECT_TRUE(hasEvent("sequence_end:S:fail"));
}

TEST_F(ProgressTest, StartEvents_Pass)
{
    lua.script("Define({ { name = 'S', tests = { { name = 'T' } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_TRUE(hasEvent("sequence_start:S:pass"));
    EXPECT_TRUE(hasEvent("test_start:T:pass"));
    EXPECT_TRUE(hasEvent("sequence_end:S:pass"));
}

TEST_F(ProgressTest, SequenceEnd_Incomplete)
{
    lua.script("Define({ { name = 'S', tests = { { name = 'T1' }, { name = 'T2' } } } })");
    lua.script(
      "Context.orchestrator.enable_list = { S = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_TRUE(hasEvent("sequence_end:S:incomplete"));
}

TEST_F(ProgressTest, DisabledSequence_SequenceEndDisabled)
{
    lua.script("Define({ { name = 'S', tests = { { name = 'T' } } } })");
    lua.script("Context.orchestrator.enable_list = { S = { enabled = false } }");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_TRUE(hasEvent("sequence_start:S:pass"));
    EXPECT_TRUE(hasEvent("sequence_end:S:disabled"));
}

TEST_F(ProgressTest, SkippedSequence_SequenceEndSkipped)
{
    lua.script(
      "Define({ { name = 'S', func = function() error(UnmetRequirement('no')) end, "
      "tests = { { name = 'T' } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_TRUE(hasEvent("sequence_end:S:skipped"));
}

// =============================================================================
// Orchestrator::toProgressResult — what the C++ binding gives the callback
// =============================================================================

class ProgressResultTest : public OrchestratorTestFixture {
protected:
    static void SetUpTestSuite() { initLogOnce(); }

    Frasy::Report::Result convert(const std::string& luaValue)
    {
        return Frasy::Lua::Orchestrator::toProgressResult("N", lua.script("return " + luaValue));
    }
};

TEST_F(ProgressResultTest, KnownNames_Converted)
{
    using Frasy::Report::Result;
    EXPECT_EQ(convert("'pass'"), Result::Pass);
    EXPECT_EQ(convert("'fail'"), Result::Fail);
    EXPECT_EQ(convert("'skipped'"), Result::Skipped);
    EXPECT_EQ(convert("'disabled'"), Result::Disabled);
    EXPECT_EQ(convert("'incomplete'"), Result::Incomplete);
}

TEST_F(ProgressResultTest, AnythingElse_Fail)
{
    using Frasy::Report::Result;
    for (const auto* value : {"'bogus'", "'PASS'", "true", "nil", "1", "{}"}) {
        EXPECT_EQ(convert(value), Result::Fail) << value;
    }
}
