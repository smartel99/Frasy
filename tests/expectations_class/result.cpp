#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// ExpectationResult.result
// =============================================================================

class ExpectationResultTest : public OrchestratorTestFixture
{
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        createTest("Seq", "T1", true);
        setStage("execution");
        lua.script("Expectation = require('lua/core/framework/expectation/expectation')");
        lua.script_file(getLuaBaseDir() + "/lua/core/framework/expectation/utils.lua");
        lua.script(R"(
            progress = {}
            __progress.report = function(kind, uut, name, sequence, test, pass)
                if kind == 'expectation' then table.insert(progress, pass) end
            end
        )");
    }

    bool progress(int index)
    {
        return lua.script("return progress[" + std::to_string(index) + "]").get<bool>();
    }

    std::string result(int index)
    {
        return lua
          .script("return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[" + std::to_string(index) +
                  "].result")
          .get<std::string>();
    }

    bool pass(int index)
    {
        return lua
          .script("return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[" + std::to_string(index) +
                  "].pass")
          .get<bool>();
    }
};

TEST_F(ExpectationResultTest, Met_IsPass)
{
    lua.script("Expectation:New(1, 'x'):ToBeEqual(1)");
    EXPECT_EQ(result(1), "pass");
    EXPECT_TRUE(pass(1));
}

TEST_F(ExpectationResultTest, Unmet_IsFail)
{
    lua.script("Expectation:New(1, 'x'):ToBeEqual(2)");
    EXPECT_EQ(result(1), "fail");
    EXPECT_FALSE(pass(1));
}

TEST_F(ExpectationResultTest, Inverted_MetIsFail)
{
    lua.script("Expectation:New(1, 'x', { inverted = true }):ToBeEqual(1)");
    EXPECT_EQ(result(1), "fail");
    EXPECT_FALSE(pass(1));
}

TEST_F(ExpectationResultTest, Inverted_UnmetIsPass)
{
    lua.script("Expectation:New(1, 'x', { inverted = true }):ToBeEqual(2)");
    EXPECT_EQ(result(1), "pass");
    EXPECT_TRUE(pass(1));
}

TEST_F(ExpectationResultTest, ReturnedResultMatchesRecorded)
{
    auto value = lua.script("return Expectation:New(5, 'x'):ToBeInRange(0, 1).result").get<std::string>();
    EXPECT_EQ(value, "fail");
    EXPECT_EQ(result(1), "fail");
}

TEST_F(ExpectationResultTest, EveryMatcher_ResultMatchesPass)
{
    lua.script(R"(
        Expectation:New(true, 'a'):ToBeTrue()
        Expectation:New(true, 'b'):ToBeFalse()
        Expectation:New(1.0, 'c'):ToBeNear(1.05, 0.1)
        Expectation:New(1.0, 'd'):ToBeInPercentage(2.0, 10)
        Expectation:New(3, 'e'):ToBeGreater(2)
        Expectation:New(3, 'f'):ToBeGreaterOrEqual(4)
        Expectation:New(3, 'g'):ToBeLesser(4)
        Expectation:New(3, 'h'):ToBeLesserOrEqual(2)
        Expectation:New('abc', 'i'):ToBeType('string')
        Expectation:New('abc', 'j'):ToMatch('^x')
    )");
    const char* expected[] = {"pass", "fail", "pass", "fail", "pass", "fail", "pass", "fail", "pass", "fail"};
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(result(i + 1), expected[i]) << i + 1;
        EXPECT_EQ(pass(i + 1), std::string(expected[i]) == "pass") << i + 1;
    }
}

TEST_F(ExpectationResultTest, PolicyFailure_RecordsFailBeforeThrowing)
{
    auto run = lua.safe_script("Expectation:New(1, 'x', { policy = ErrorPolicy.stopCurrent }):ToBeEqual(2)",
                               sol::script_pass_on_error);
    EXPECT_FALSE(run.valid());
    EXPECT_EQ(result(1), "fail");
}

TEST_F(ExpectationResultTest, Progress_ReportsInvertedOutcome)
{
    lua.script(R"(
        Expectation:New(1, 'a'):ToBeEqual(1)
        Expectation:New(1, 'b', { inverted = true }):ToBeEqual(1)
        Expectation:New(1, 'c', { inverted = true }):ToBeEqual(2)
    )");
    EXPECT_TRUE(progress(1));
    EXPECT_FALSE(progress(2));
    EXPECT_TRUE(progress(3));
}

TEST_F(ExpectationResultTest, HandBuiltEntry_ResultFilledFromPass)
{
    lua.script(R"(
        Orchestrator.AddExpectationResult({ name = 'a', pass = true })
        Orchestrator.AddExpectationResult({ name = 'b', pass = false })
        Orchestrator.AddExpectationResult({ name = 'c' })
    )");
    EXPECT_EQ(result(1), "pass");
    EXPECT_TRUE(progress(1));
    EXPECT_EQ(result(2), "fail");
    EXPECT_FALSE(progress(2));
    EXPECT_EQ(result(3), "fail");
    EXPECT_FALSE(progress(3));
}

TEST_F(ExpectationResultTest, HandBuiltEntry_ExplicitResultWins)
{
    lua.script("Orchestrator.AddExpectationResult({ name = 'a', result = 'fail', pass = true })");
    EXPECT_EQ(result(1), "fail");
    EXPECT_FALSE(progress(1));
}
