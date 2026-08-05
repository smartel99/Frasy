#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class ExpectationInvertedTest : public OrchestratorTestFixture
{
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        createTest("Seq", "T1", true);
        setStage("execution");
        lua.script("Expectation = require('lua/core/framework/expectation/expectation')");
        lua.script_file(getLuaBaseDir() + "/lua/core/framework/expectation/utils.lua");
    }
};

TEST_F(ExpectationInvertedTest, Inverted_SetsInvertedFlag)
{
    auto inverted = lua.script(R"(
        local e = Expectation:New(true, "x", {inverted = true})
        return e.inverted
    )").get<bool>();
    EXPECT_TRUE(inverted);
}

TEST_F(ExpectationInvertedTest, Inverted_InvertsPassResult)
{
    // Without Inverted: true ToBeEqual true -> pass
    lua.script("Expectation:New(true, 'a'):ToBeEqual(true)");
    EXPECT_TRUE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].pass").get<bool>());

    // With Inverted: true ToBeEqual true -> pass is true, but inverted is true
    // So the "effective" result is fail (pass == inverted)
    lua.script("Expectation:New(true, 'b', {inverted=true}):ToBeEqual(true)");
    EXPECT_FALSE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[2].pass").get<bool>());
    EXPECT_TRUE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[2].inverted").get<bool>());
}

TEST_F(ExpectationInvertedTest, Inverted_ToBeEqual_FailBecomesEffectivePass)
{
    // Without Inverted: 1 ToBeEqual 2 -> fail
    // With Inverted: 1 ToBeEqual 2 -> fail, inverted=true -> effective pass
    lua.script("Expectation:New(1, 'x', {inverted=true}):ToBeEqual(2)");
    EXPECT_TRUE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].pass").get<bool>());
    EXPECT_TRUE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].inverted").get<bool>());
    // pass != inverted means effectively passing
}
