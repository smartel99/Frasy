#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// HasPassed / HasBeenSkipped
// =============================================================================

TEST_F(OrchestratorTestFixture, HasPassed_SequenceLevel)
{
    createSequence("Seq");
    // Default result: fail
    EXPECT_FALSE(lua.script(
        "return Orchestrator.HasPassed(require('lua/core/framework/scope'):New('Seq'))").get<bool>());

    lua.script("Context.orchestrator.sequences['Seq'].result:Set('pass')");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasPassed(require('lua/core/framework/scope'):New('Seq'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasPassed_FalseUnlessPass)
{
    createSequence("Seq");
    for (const char* value : {"fail", "skipped", "disabled", "incomplete"}) {
        lua.script(std::string("Context.orchestrator.sequences['Seq'].result:Set('") + value + "')");
        EXPECT_FALSE(lua.script(
            "return Orchestrator.HasPassed(require('lua/core/framework/scope'):New('Seq'))").get<bool>())
          << value;
    }
}

TEST_F(OrchestratorTestFixture, HasPassed_TestLevel)
{
    createTest("Seq", "T1");
    lua.script("Context.orchestrator.sequences['Seq'].tests['T1'].result:Set('pass')");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasPassed(require('lua/core/framework/scope'):New('Seq', 'T1'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasPassed_ThrowsForUnknownScope)
{
    sol::protected_function_result result = lua.safe_script(
        "Orchestrator.HasPassed(require('lua/core/framework/scope'):New('NoSuch'))", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(OrchestratorTestFixture, HasBeenSkipped_False)
{
    createSequence("Seq");
    EXPECT_FALSE(lua.script(
        "return Orchestrator.HasBeenSkipped(require('lua/core/framework/scope'):New('Seq'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasBeenSkipped_TrueForSkippedAndDisabled)
{
    createSequence("Seq");
    for (const char* value : {"skipped", "disabled"}) {
        lua.script(std::string("Context.orchestrator.sequences['Seq'].result:Set('") + value + "')");
        EXPECT_TRUE(lua.script(
            "return Orchestrator.HasBeenSkipped(require('lua/core/framework/scope'):New('Seq'))").get<bool>())
          << value;
    }
}

TEST_F(OrchestratorTestFixture, HasBeenSkipped_FalseForIncomplete)
{
    createSequence("Seq");
    lua.script("Context.orchestrator.sequences['Seq'].result:Set('incomplete')");
    EXPECT_FALSE(lua.script(
        "return Orchestrator.HasBeenSkipped(require('lua/core/framework/scope'):New('Seq'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasBeenSkipped_ThrowsForUnknownScope)
{
    sol::protected_function_result result = lua.safe_script(
        "Orchestrator.HasBeenSkipped(require('lua/core/framework/scope'):New('NoSuch'))", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}
