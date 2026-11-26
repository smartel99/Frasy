#include "execution_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// RunSequence / RunTest — results set during execution
// =============================================================================

TEST_F(ExecutionTest, AllPass)
{
    lua.script("Define({ { name = 'S', tests = { { name = 'T1' }, { name = 'T2' } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(result("S", "T1"), "pass");
    EXPECT_EQ(result("S", "T2"), "pass");
    EXPECT_EQ(result("S"), "pass");
    EXPECT_TRUE(field("S", "T1", "pass"));
    EXPECT_FALSE(field("S", "T1", "skipped"));
    EXPECT_TRUE(field("S", "T1", "enabled"));
    EXPECT_TRUE(query("HasPassed", "S"));
}

TEST_F(ExecutionTest, FailedExpectation_FailsTestAndSequence)
{
    lua.script("Define({ { name = 'S', tests = { { name = 'T1', func = Fail }, { name = 'T2' } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(result("S", "T1"), "fail");
    EXPECT_EQ(reason("S", "T1"), "Unmet expectation");
    EXPECT_EQ(result("S", "T2"), "pass");
    EXPECT_EQ(result("S"), "fail");
    EXPECT_FALSE(field("S", "T1", "pass"));
    EXPECT_FALSE(query("HasPassed", "S"));
}

TEST_F(ExecutionTest, RealExpectations_DecideTestResult)
{
    lua.script(R"(
        local Expectation = require('lua/core/framework/expectation/expectation')
        Define({ { name = 'S', tests = {
            { name = 'Met', func = function() Expectation:New(1, 'x'):ToBeEqual(1) end },
            { name = 'Unmet', func = function() Expectation:New(1, 'x'):ToBeEqual(2) end },
            { name = 'InvertedUnmet', func = function()
                Expectation:New(1, 'x', { inverted = true }):ToBeEqual(2)
            end },
        } } })
    )");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(result("S", "Met"), "pass");
    EXPECT_EQ(result("S", "Unmet"), "fail");
    EXPECT_EQ(reason("S", "Unmet"), "Unmet expectation");
    EXPECT_EQ(result("S", "InvertedUnmet"), "pass");
    EXPECT_EQ(result("S"), "fail");
}

TEST_F(ExecutionTest, UnmetRequirement_SkipsTestOnly)
{
    lua.script(R"(Define({ { name = 'S', tests = {
        { name = 'T1', func = function() error(UnmetRequirement('needs X')) end },
        { name = 'T2' },
    } } }))");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(result("S", "T1"), "skipped");
    // The reason says what was required, without a traceback.
    EXPECT_EQ(reason("S", "T1"), "UnmetRequirement: needs X");
    EXPECT_TRUE(field("S", "T1", "skipped"));
    EXPECT_TRUE(field("S", "T1", "enabled"));
    EXPECT_TRUE(query("HasBeenSkipped", "S", "T1"));
    EXPECT_FALSE(query("HasPassed", "S", "T1"));
    EXPECT_EQ(result("S"), "pass");
}

TEST_F(ExecutionTest, FailedExpectationThenUnmetRequirement_Fails)
{
    lua.script(R"(Define({ { name = 'S', tests = {
        { name = 'T1', func = function() Fail(); error(UnmetRequirement('late')) end },
    } } }))");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(result("S", "T1"), "fail");
    EXPECT_EQ(reason("S", "T1"), "Unmet expectation");
    EXPECT_EQ(result("S"), "fail");
}

TEST_F(ExecutionTest, RequirementOnDisabledTest_SkipsDependent)
{
    lua.script(R"(Define({ { name = 'S', tests = {
        { name = 'T1' },
        { name = 'T2', func = function()
            local ScopeRequirement = require('lua/core/framework/scope_requirement/execution')
            local Scope = require('lua/core/framework/scope')
            Requires(ScopeRequirement:New(Orchestrator, Scope:New('S', 'T1')):ToPass())
        end },
    } } }))");
    lua.script(
      "Context.orchestrator.enable_list = { S = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(result("S", "T1"), "disabled");
    EXPECT_EQ(result("S", "T2"), "skipped");
    EXPECT_EQ(result("S"), "incomplete");
}

TEST_F(ExecutionTest, AllTestsSkipped_SequencePasses)
{
    lua.script(R"(Define({ { name = 'S', tests = {
        { name = 'T1', func = function() error(UnmetRequirement('no')) end },
    } } }))");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(result("S", "T1"), "skipped");
    EXPECT_EQ(result("S"), "pass");
}

TEST_F(ExecutionTest, DisabledTest_SequenceIncomplete)
{
    lua.script("Define({ { name = 'S', tests = { { name = 'T1' }, { name = 'T2' } } } })");
    lua.script(
      "Context.orchestrator.enable_list = { S = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_FALSE(ran("S", "T1"));
    EXPECT_EQ(result("S", "T1"), "disabled");
    EXPECT_EQ(reason("S", "T1"), "Disabled");
    EXPECT_FALSE(field("S", "T1", "enabled"));
    EXPECT_TRUE(field("S", "T1", "skipped"));
    EXPECT_TRUE(query("HasBeenSkipped", "S", "T1"));
    EXPECT_EQ(result("S", "T2"), "pass");
    EXPECT_EQ(result("S"), "incomplete");
    EXPECT_FALSE(query("HasPassed", "S"));
    EXPECT_FALSE(query("HasBeenSkipped", "S"));
}

TEST_F(ExecutionTest, DisabledTestAndFailure_SequenceFails)
{
    lua.script("Define({ { name = 'S', tests = { { name = 'T1' }, { name = 'T2', func = Fail } } } })");
    lua.script(
      "Context.orchestrator.enable_list = { S = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(result("S", "T1"), "disabled");
    EXPECT_EQ(result("S"), "fail");
}

TEST_F(ExecutionTest, DisabledSequence_TestsInheritDisabled)
{
    lua.script(R"(
        sequenceRan = false
        Define({ { name = 'S', func = function() sequenceRan = true end,
                   tests = { { name = 'T1' }, { name = 'T2' } } } })
        Context.orchestrator.enable_list = { S = { enabled = false } }
    )");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_FALSE(lua.script("return sequenceRan").get<bool>());
    EXPECT_EQ(result("S"), "disabled");
    for (const char* test : {"T1", "T2"}) {
        EXPECT_FALSE(ran("S", test)) << test;
        EXPECT_EQ(result("S", test), "disabled") << test;
        EXPECT_EQ(reason("S", test), "Disabled") << test;
        EXPECT_FALSE(field("S", test, "enabled")) << test;
    }
    EXPECT_TRUE(query("HasBeenSkipped", "S"));
}

TEST_F(ExecutionTest, SkippedSequence_TestsInheritSkipped)
{
    lua.script(R"(Define({ { name = 'S', func = function() error(UnmetRequirement('no fixture')) end,
                            tests = { { name = 'T1' }, { name = 'T2' } } } }))");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(result("S"), "skipped");
    EXPECT_EQ(reason("S"), "UnmetRequirement: no fixture");
    for (const char* test : {"T1", "T2"}) {
        EXPECT_FALSE(ran("S", test)) << test;
        EXPECT_EQ(result("S", test), "skipped") << test;
        EXPECT_EQ(reason("S", test), reason("S")) << test;
        EXPECT_TRUE(field("S", test, "enabled")) << test;
    }
}

TEST_F(ExecutionTest, SequenceError_FailsSequenceWithStringReason)
{
    lua.script("Define({ { name = 'S', func = function() error('boom') end, tests = { { name = 'T1' } } } })");
    EXPECT_FALSE(runSection());
    EXPECT_EQ(result("S"), "fail");
    EXPECT_TRUE(lua.script("return type(ResultOf('S').reason) == 'string'").get<bool>());
    EXPECT_NE(reason("S").find("boom"), std::string::npos);
    EXPECT_FALSE(ran("S", "T1"));
}

TEST_F(ExecutionTest, TestError_FailsTestWithStringReason)
{
    lua.script(R"(Define({ { name = 'S', tests = {
        { name = 'T1', func = function() error('boom') end },
        { name = 'T2' },
    } } }))");
    EXPECT_FALSE(runSection());
    EXPECT_EQ(result("S", "T1"), "fail");
    EXPECT_TRUE(lua.script("return type(ResultOf('S', 'T1').reason) == 'string'").get<bool>());
    EXPECT_NE(reason("S", "T1").find("boom"), std::string::npos);
    EXPECT_EQ(teamFailCount, 1);
    // A test cut off by the error keeps the default result.
    EXPECT_FALSE(ran("S", "T2"));
    EXPECT_EQ(result("S", "T2"), "fail");
}

TEST_F(ExecutionTest, StopCurrentExpectation_FailsTestAndContinues)
{
    lua.script(R"(Define({ { name = 'S', tests = {
        { name = 'T1', func = function() error(UnmetExpectation(ErrorPolicy.stopCurrent, 'limit')) end },
        { name = 'T2' },
    } } }))");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(result("S", "T1"), "fail");
    EXPECT_NE(reason("S", "T1").find("limit"), std::string::npos);
    EXPECT_TRUE(ran("S", "T2"));
    EXPECT_EQ(result("S", "T2"), "pass");
    EXPECT_EQ(result("S"), "fail");
}

TEST_F(ExecutionTest, StopAllExpectation_FailsTestAndStops)
{
    lua.script(R"(Define({ { name = 'S', tests = {
        { name = 'T1', func = function() error(UnmetExpectation(ErrorPolicy.stopAll, 'halt')) end },
        { name = 'T2' },
    } } }))");
    EXPECT_FALSE(runSection());
    EXPECT_EQ(result("S", "T1"), "fail");
    EXPECT_NE(reason("S", "T1").find("halt"), std::string::npos);
    EXPECT_FALSE(ran("S", "T2"));
}

TEST_F(ExecutionTest, PendingTestsOfLaterSections_AreNotCounted)
{
    lua.script(R"(Define({ { name = 'S', tests = {
        { name = 'T1' },
        { name = 'T2', section = 2 },
    } } }))");
    lua.script(
      "Context.orchestrator.enable_list = { S = { enabled = true, T1 = { enabled = true }, "
      "T2 = { enabled = false } } }");
    ASSERT_TRUE(runSection(1)) << lastError;
    EXPECT_EQ(result("S"), "pass");
    ASSERT_TRUE(runSection(2)) << lastError;
    EXPECT_EQ(result("S", "T2"), "disabled");
    EXPECT_EQ(result("S"), "incomplete");
}

TEST_F(ExecutionTest, DisabledSequenceAcrossSections_StaysDisabled)
{
    lua.script(R"(
        sequenceRuns = 0
        Define({ { name = 'S', func = function() sequenceRuns = sequenceRuns + 1 end,
                   tests = { { name = 'T1' }, { name = 'T2', section = 2 } } } })
        Context.orchestrator.enable_list = { S = { enabled = false } }
    )");
    ASSERT_TRUE(runSection(1)) << lastError;
    ASSERT_TRUE(runSection(2)) << lastError;
    EXPECT_EQ(lua.script("return sequenceRuns").get<int>(), 0);
    EXPECT_EQ(result("S"), "disabled");
    EXPECT_FALSE(ran("S", "T2"));
    EXPECT_EQ(result("S", "T2"), "disabled");
}

TEST_F(ExecutionTest, SkippedSequenceAcrossSections_RunsBodyOnce)
{
    lua.script(R"(
        sequenceRuns = 0
        Define({ { name = 'S', func = function()
                       sequenceRuns = sequenceRuns + 1
                       error(UnmetRequirement('no'))
                   end,
                   tests = { { name = 'T1' }, { name = 'T2', section = 2 } } } })
    )");
    ASSERT_TRUE(runSection(1)) << lastError;
    ASSERT_TRUE(runSection(2)) << lastError;
    EXPECT_EQ(lua.script("return sequenceRuns").get<int>(), 1);
    EXPECT_EQ(result("S"), "skipped");
    EXPECT_FALSE(ran("S", "T2"));
    EXPECT_EQ(result("S", "T2"), "skipped");
}

TEST_F(ExecutionTest, SeveralSequences_AreIndependent)
{
    lua.script(R"(Define({
        { name = 'A', tests = { { name = 'T1', func = Fail } } },
        { name = 'B', tests = { { name = 'T1' } } },
        { name = 'C', tests = { { name = 'T1' } } },
    }))");
    lua.script("Context.orchestrator.enable_list = { C = { enabled = false } }");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(result("A"), "fail");
    EXPECT_EQ(result("B"), "pass");
    EXPECT_EQ(result("C"), "disabled");
}

// =============================================================================
// Team.Sync inside RunTest — real team.lua, host side mocked through Team.__sync
// =============================================================================

class TeamExecutionTest : public ExecutionTest {
protected:
    void SetUp() override
    {
        ExecutionTest::SetUp();
        lua.script_file(getLuaBaseDir() + "/lua/core/sdk/environment/team.lua");
        lua.script("Context.team.hasTeam = true");
    }
};

TEST_F(TeamExecutionTest, TeammateFailure_FailsPassingTest)
{
    lua.script("function Team.__sync(status) return Team.Status.fail end");
    lua.script("Define({ { name = 'S', tests = { { name = 'T1' } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(result("S", "T1"), "fail");
    EXPECT_EQ(reason("S", "T1"), "Teammate failure");
    EXPECT_EQ(result("S"), "fail");
}

TEST_F(TeamExecutionTest, TeammateCriticalFailure_FailsTestAndStops)
{
    lua.script("function Team.__sync(status) return Team.Status.critical_failure end");
    lua.script("Define({ { name = 'S', tests = { { name = 'T1' }, { name = 'T2' } } } })");
    EXPECT_FALSE(runSection());
    EXPECT_EQ(result("S", "T1"), "fail");
    EXPECT_EQ(reason("S", "T1"), "Teammate critical failure");
    EXPECT_FALSE(ran("S", "T2"));
}

TEST_F(TeamExecutionTest, SkippedTest_SentAsFail)
{
    lua.script("sent = {}; function Team.__sync(status) table.insert(sent, status); return status end");
    lua.script(R"(Define({ { name = 'S', tests = {
        { name = 'T1', func = function() error(UnmetRequirement('no')) end },
    } } }))");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(lua.script("return sent[1]").get<int>(), 1);
    EXPECT_EQ(result("S", "T1"), "skipped");
}

TEST_F(ExecutionTest, ErrorReason_KeepsTraceback)
{
    lua.script(R"(Define({ { name = 'S', tests = {
        { name = 'T1', func = function() error(UnmetExpectation(ErrorPolicy.stopCurrent, 'bad value')) end },
    } } }))");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_NE(reason("S", "T1").find("bad value"), std::string::npos);
    EXPECT_NE(reason("S", "T1").find("stack traceback:"), std::string::npos);
}

TEST_F(ExecutionTest, RequirementWithoutReason_SkipReasonIsItsLocation)
{
    lua.script(R"(
Define({ { name = 'S', tests = {
    { name = 'T1', func = function() Requires(RequirementSpecifier(function() return false end)) end },
} } }))",
               "@C:/frasy/lua/user/product/test.lua");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_EQ(result("S", "T1"), "skipped");
    EXPECT_EQ(reason("S", "T1"), "UnmetRequirement: requirement at product/test.lua:3");
}
