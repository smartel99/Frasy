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

    std::size_t count(const std::string& event)
    {
        const auto all = events();
        return static_cast<std::size_t>(std::ranges::count(all, event));
    }

    /** Runs a section that must raise; returns the message of what it raised. */
    std::string raisedBy(int section = 1)
    {
        return lua
          .script("local ok, err = pcall(Orchestrator.ExecuteSection, " + std::to_string(section) +
                  ")\n"
                  "assert(not ok, 'section did not raise')\n"
                  "return type(err) == 'table' and tostring(err.what) or tostring(err)")
          .get<std::string>();
    }

    static bool contains(const std::string& text, const std::string& part)
    {
        return text.find(part) != std::string::npos;
    }
};

TEST_F(ProgressTest, TestEnd_SentOncePerTest)
{
    lua.script(R"(Define({ { name = 'S', tests = {
        { name = 'Skip', func = function() error(UnmetRequirement('no')) end },
        { name = 'Bad', func = Fail },
        { name = 'Ok' },
    } } }))");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(count("test_end:Skip:skipped"), 1U);
    EXPECT_EQ(count("test_end:Bad:fail"), 1U);
    EXPECT_EQ(count("test_end:Ok:pass"), 1U);
}

/** A test that stops the run still ends, once. */
class ProgressStopTest : public ProgressTest, public ::testing::WithParamInterface<const char*> {};

TEST_P(ProgressStopTest, TestEnd_Fail)
{
    lua.script(std::string("Define({ { name = 'S', tests = { { name = 'T', func = function() ") + GetParam() +
               " end }, { name = 'After' } } } })");
    const auto raised = raisedBy();
    // The original error still reaches the caller.
    EXPECT_TRUE(contains(raised, "boom") || contains(raised, "stop all")) << raised;
    EXPECT_EQ(count("test_start:T:pass"), 1U);
    EXPECT_EQ(count("test_end:T:fail"), 1U);
    EXPECT_EQ(result("S", "T"), "fail");
    EXPECT_EQ(count("sequence_end:S:fail"), 1U);
    EXPECT_FALSE(hasEvent("test_start:After:pass"));
}

TEST_F(ProgressTest, StopCurrent_TestEndsAndRunGoesOn)
{
    lua.script(
      "Define({ { name = 'S', tests = { { name = 'T', func = function() error(UnmetExpectation(1, 'stop current')) "
      "end }, { name = 'After' } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(count("test_end:T:fail"), 1U);
    EXPECT_EQ(count("test_end:After:pass"), 1U);
    EXPECT_EQ(count("sequence_end:S:fail"), 1U);
}

TEST_F(ProgressTest, TeammateCriticalFailure_TestEndsThenRaises)
{
    lua.script(R"(function Team.Sync(result)
        result:Set('fail', 'Teammate critical failure')
        error(TeamError('Teammate critical failure'))
    end)");
    lua.script("Define({ { name = 'S', tests = { { name = 'T' }, { name = 'After' } } } })");
    const auto raised = raisedBy();
    EXPECT_TRUE(contains(raised, "Teammate critical failure")) << raised;
    // Raised outside the test body: where it came from is kept.
    EXPECT_TRUE(contains(raised, "stack traceback:")) << raised;
    EXPECT_EQ(count("test_end:T:fail"), 1U);
    EXPECT_EQ(count("sequence_end:S:fail"), 1U);
    EXPECT_FALSE(hasEvent("test_start:After:pass"));
}

TEST_F(ProgressTest, ErrorOutsideTestBody_TracebackKept)
{
    lua.script("function Team.Sync(result) error('sync broke') end");
    lua.script("Define({ { name = 'S', tests = { { name = 'T' } } } })");
    const auto raised = raisedBy();
    EXPECT_TRUE(contains(raised, "sync broke")) << raised;
    EXPECT_TRUE(contains(raised, "stack traceback:")) << raised;
    EXPECT_EQ(count("test_end:T:pass"), 1U);
}

TEST_F(ProgressTest, SequenceBodyRaises_SequenceEndFail)
{
    lua.script("Define({ { name = 'S', func = function() error('boom') end, tests = { { name = 'T' } } } })");
    EXPECT_TRUE(contains(raisedBy(), "boom"));
    EXPECT_EQ(count("sequence_start:S:pass"), 1U);
    EXPECT_EQ(count("sequence_end:S:fail"), 1U);
    EXPECT_EQ(result("S"), "fail");
}

TEST_F(ProgressTest, TestRaisesInLaterSection_SequenceEndOnce)
{
    lua.script(
      "Define({ { name = 'S', tests = { { name = 'T1' }, { name = 'T2', section = 2, "
      "func = function() error('boom') end } } } })");
    ASSERT_TRUE(runSection(1)) << lastError;
    EXPECT_EQ(count("sequence_end:S:pass") + count("sequence_end:S:fail"), 0U);
    EXPECT_FALSE(runSection(2));
    EXPECT_EQ(count("sequence_end:S:fail"), 1U);
    EXPECT_EQ(count("test_end:T2:fail"), 1U);
    // The report keeps the result of the sections that completed.
    EXPECT_EQ(result("S"), "pass");
}

INSTANTIATE_TEST_SUITE_P(Raises,
                         ProgressStopTest,
                         ::testing::Values("error('boom')",
                                           "error(UnmetExpectation(2, 'stop all'))",
                                           "error(GenericError('boom'))"));

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
