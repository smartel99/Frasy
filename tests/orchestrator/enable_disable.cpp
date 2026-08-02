#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// Enable / Disable
// =============================================================================

TEST_F(OrchestratorTestFixture, Enable_Sequence)
{
    createSequence("Seq");
    lua.script("Orchestrator.Enable('Seq')");
    auto enabled = lua.script("return Context.orchestrator.enable_list['Seq'].enabled").get<bool>();
    EXPECT_TRUE(enabled);
}

TEST_F(OrchestratorTestFixture, Disable_Sequence)
{
    createSequence("Seq");
    lua.script("Orchestrator.Disable('Seq')");
    auto enabled = lua.script("return Context.orchestrator.enable_list['Seq'].enabled").get<bool>();
    EXPECT_FALSE(enabled);
}

TEST_F(OrchestratorTestFixture, Enable_Test)
{
    createTest("Seq", "T1");
    lua.script("Orchestrator.Enable('Seq', 'T1')");
    auto enabled = lua.script("return Context.orchestrator.enable_list['Seq']['T1']").get<bool>();
    EXPECT_TRUE(enabled);
}

TEST_F(OrchestratorTestFixture, Disable_Test)
{
    createTest("Seq", "T1");
    lua.script("Orchestrator.Disable('Seq', 'T1')");
    auto enabled = lua.script("return Context.orchestrator.enable_list['Seq']['T1']").get<bool>();
    EXPECT_FALSE(enabled);
}

TEST_F(OrchestratorTestFixture, Enable_UnknownScopeThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "Orchestrator.Enable('NoSuch')", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(OrchestratorTestFixture, Disable_UnknownScopeThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "Orchestrator.Disable('NoSuch')", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(OrchestratorTestFixture, Disable_UnknownTestThrows)
{
    createSequence("Seq");
    sol::protected_function_result result = lua.safe_script(
        "Orchestrator.Disable('Seq', 'NoSuchTest')", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}
