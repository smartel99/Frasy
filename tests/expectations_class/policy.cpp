#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class ExpectationPolicyTest : public OrchestratorTestFixture {
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

TEST_F(ExpectationPolicyTest, Policy_SetsPolicyFlag)
{
    auto policy = lua
                    .script(R"(
        local e = Expectation:New(1, "x", {policy=ErrorPolicy.stopCurrent})
        return e.policy
    )")
                    .get<int>();
    EXPECT_EQ(policy, 1);

    policy = lua
               .script(R"(
        local e = Expectation:New(1, "x", {policy=ErrorPolicy.stopAll})
        return e.policy
    )")
               .get<int>();
    EXPECT_EQ(policy, 2);

    auto nopolicy = lua
                      .script(R"(
        local e = Expectation:New(1, "x")
        return e.policy
    )")
                      .get<std::optional<int>>();
    EXPECT_FALSE(nopolicy.has_value());
}

TEST_F(ExpectationPolicyTest, Policy_PassingDoesNotThrow)
{
    // pass=true, inverted=false -> pass != inverted -> no throw
    sol::protected_function_result result = lua.safe_script(R"(
        Expectation:New(true, "x", {policy=ErrorPolicy.stopCurrent}):ToBeTrue()
    )",
                                                            sol::script_pass_on_error);
    EXPECT_TRUE(result.valid());
}

TEST_F(ExpectationPolicyTest, Policy_FailingThrowsUnmetExpectation)
{
    // pass=false, inverted=false -> pass == inverted -> throw
    sol::protected_function_result result = lua.safe_script(R"(
        Expectation:New(false, "x", {policy=ErrorPolicy.stopCurrent}):ToBeTrue()
    )",
                                                            sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(ExpectationPolicyTest, Policy_NotInverted_PassingThrows)
{
    // With Not: pass=true, inverted=true -> pass == inverted -> throw
    sol::protected_function_result result = lua.safe_script(R"(
        Expectation:New(true, "x", {inverted=true, policy=ErrorPolicy.stopCurrent}):ToBeTrue()
    )",
                                                            sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(ExpectationPolicyTest, Policy_NotInverted_FailingDoesNotThrow)
{
    // With Not: pass=false, inverted=true -> pass != inverted -> no throw
    sol::protected_function_result result = lua.safe_script(R"(
        Expectation:New(false, "x", {inverted=true, policy=ErrorPolicy.stopCurrent}):ToBeTrue()
    )",
                                                            sol::script_pass_on_error);
    EXPECT_TRUE(result.valid());
}