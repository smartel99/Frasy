#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// CreateTest
// =============================================================================

TEST_F(OrchestratorTestFixture, CreateTest_RegistersTest)
{
    createSequence("Seq", true);
    lua.script("Orchestrator.CreateTest('T1', function() end, 'test', 0)");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasTest(require('lua/core/framework/scope'):New('Seq', 'T1'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, CreateTest_OutsideSequenceThrows)
{
    // No scope set
    lua.script("Context.orchestrator.scope = nil");
    sol::protected_function_result result = lua.safe_script(
        "Orchestrator.CreateTest('T1', function() end, 'test', 0)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(OrchestratorTestFixture, CreateTest_WhileInTestThrows)
{
    createSequence("Seq", true);
    lua.script("Orchestrator.CreateTest('T1', function() end, 'test', 0)");
    // Set scope to be inside the test
    lua.script("Context.orchestrator.scope = require('lua/core/framework/scope'):New('Seq', 'T1')");
    sol::protected_function_result result = lua.safe_script(
        "Orchestrator.CreateTest('T2', function() end, 'test', 0)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(OrchestratorTestFixture, CreateTest_InitializesValuesTable)
{
    createSequence("Seq", true);
    lua.script("Orchestrator.CreateTest('T1', function() end, 'test', 0)");
    auto result = lua.script("return type(Context.orchestrator.values['Seq']['T1'])").get<std::string>();
    EXPECT_EQ(result, "table");
}
