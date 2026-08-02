#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// HasPassed / HasBeenSkipped
// =============================================================================

TEST_F(OrchestratorTestFixture, HasPassed_SequenceLevel)
{
    createSequence("Seq");
    // Default result: pass=false, skipped=false
    EXPECT_FALSE(lua.script(
        "return Orchestrator.HasPassed(require('lua/core/framework/scope'):New('Seq'))").get<bool>());

    // Set pass to true
    lua.script("Context.orchestrator.sequences['Seq'].result.pass = true");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasPassed(require('lua/core/framework/scope'):New('Seq'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasPassed_SkippedSequenceReturnsFalse)
{
    createSequence("Seq");
    lua.script("Context.orchestrator.sequences['Seq'].result.pass = true");
    lua.script("Context.orchestrator.sequences['Seq'].result.skipped = true");
    EXPECT_FALSE(lua.script(
        "return Orchestrator.HasPassed(require('lua/core/framework/scope'):New('Seq'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasPassed_TestLevel)
{
    createTest("Seq", "T1");
    lua.script("Context.orchestrator.sequences['Seq'].tests['T1'].result.pass = true");
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

TEST_F(OrchestratorTestFixture, HasBeenSkipped_True)
{
    createSequence("Seq");
    lua.script("Context.orchestrator.sequences['Seq'].result.skipped = true");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasBeenSkipped(require('lua/core/framework/scope'):New('Seq'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasBeenSkipped_ThrowsForUnknownScope)
{
    sol::protected_function_result result = lua.safe_script(
        "Orchestrator.HasBeenSkipped(require('lua/core/framework/scope'):New('NoSuch'))", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}
