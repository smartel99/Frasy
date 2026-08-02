#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class ExpectationNotTest : public OrchestratorTestFixture
{
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        createTest("Seq", "T1", true);
        setStage("execution");
        lua.script("Expectation = require('lua/core/framework/expectation/execution')");
        lua.script_file(getLuaBaseDir() + "/lua/core/framework/expectation/common.lua");
    }
};

TEST_F(ExpectationNotTest, Not_SetsInvertedFlag)
{
    auto inverted = lua.script(R"(
        local e = Expectation:New(true, "x"):Not()
        return e.result.inverted
    )").get<bool>();
    EXPECT_TRUE(inverted);
}

TEST_F(ExpectationNotTest, Not_DoubleNotCancels)
{
    auto inverted = lua.script(R"(
        local e = Expectation:New(true, "x"):Not():Not()
        return e.result.inverted
    )").get<bool>();
    EXPECT_FALSE(inverted);
}

TEST_F(ExpectationNotTest, Not_InvertsPassResult)
{
    // Without Not: true ToBeEqual true -> pass
    lua.script("Expectation:New(true, 'a'):ToBeEqual(true)");
    EXPECT_TRUE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].pass").get<bool>());

    // With Not: true ToBeEqual true -> pass is true, but inverted is true
    // So the "effective" result is fail (pass == inverted)
    lua.script("Expectation:New(true, 'b'):Not():ToBeEqual(true)");
    EXPECT_TRUE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[2].pass").get<bool>());
    EXPECT_TRUE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[2].inverted").get<bool>());
}

TEST_F(ExpectationNotTest, Not_ToBeEqual_FailBecomesEffectivePass)
{
    // Without Not: 1 ToBeEqual 2 -> fail
    // With Not: 1 ToBeEqual 2 -> fail, inverted=true -> effective pass
    lua.script("Expectation:New(1, 'x'):Not():ToBeEqual(2)");
    EXPECT_FALSE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].pass").get<bool>());
    EXPECT_TRUE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].inverted").get<bool>());
    // pass != inverted means effectively passing
}
