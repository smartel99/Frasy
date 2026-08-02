#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// CreateSequence
// =============================================================================

TEST_F(OrchestratorTestFixture, CreateSequence_RegistersSequence)
{
    createSequence("MySeq");
    EXPECT_TRUE(lua.script("return Orchestrator.HasSequence(require('lua/core/framework/scope'):New('MySeq'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, CreateSequence_DuplicateThrows)
{
    createSequence("MySeq");
    sol::protected_function_result result = lua.safe_script(
        "Orchestrator.CreateSequence('MySeq', function() end, 'test', 0)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(OrchestratorTestFixture, CreateSequence_WhileInSequenceThrows)
{
    createSequence("SeqA", true); // enters scope
    sol::protected_function_result result = lua.safe_script(
        "Orchestrator.CreateSequence('SeqB', function() end, 'test', 0)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(OrchestratorTestFixture, CreateSequence_InitializesValuesTable)
{
    createSequence("MySeq");
    auto result = lua.script("return type(Context.orchestrator.values['MySeq'])").get<std::string>();
    EXPECT_EQ(result, "table");
}
