#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class ExpectationMandatoryTest : public OrchestratorTestFixture
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

TEST_F(ExpectationMandatoryTest, Mandatory_SetsMandatoryFlag)
{
    auto mandatory = lua.script(R"(
        local e = Expectation:New(1, "x", {mandatory=true})
        return e.mandatory
    )").get<bool>();
    EXPECT_TRUE(mandatory);
}

TEST_F(ExpectationMandatoryTest, Mandatory_PassingDoesNotThrow)
{
    // pass=true, inverted=false -> pass != inverted -> no throw
    sol::protected_function_result result = lua.safe_script(R"(
        Expectation:New(true, "x", {mandatory=true}):ToBeTrue()
    )", sol::script_pass_on_error);
    EXPECT_TRUE(result.valid());
}

TEST_F(ExpectationMandatoryTest, Mandatory_FailingThrowsUnmetExpectation)
{
    // pass=false, inverted=false -> pass == inverted -> throw
    sol::protected_function_result result = lua.safe_script(R"(
        Expectation:New(false, "x", {mandatory=true}):ToBeTrue()
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(ExpectationMandatoryTest, Mandatory_NotInverted_PassingThrows)
{
    // With Not: pass=true, inverted=true -> pass == inverted -> throw
    sol::protected_function_result result = lua.safe_script(R"(
        Expectation:New(true, "x", {inverted=true, mandatory=true}):ToBeTrue()
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(ExpectationMandatoryTest, Mandatory_NotInverted_FailingDoesNotThrow)
{
    // With Not: pass=false, inverted=true -> pass != inverted -> no throw
    sol::protected_function_result result = lua.safe_script(R"(
        Expectation:New(false, "x", {inverted=true, mandatory=true}):ToBeTrue()
    )", sol::script_pass_on_error);
    EXPECT_TRUE(result.valid());
}

TEST_F(ExpectationMandatoryTest, NonMandatory_FailingDoesNotThrow)
{
    // Without Mandatory, failing should NOT throw
    sol::protected_function_result result = lua.safe_script(R"(
        Expectation:New(false, "x"):ToBeTrue()
    )", sol::script_pass_on_error);
    EXPECT_TRUE(result.valid());
}
