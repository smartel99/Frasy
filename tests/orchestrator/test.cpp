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

// =============================================================================
// IsInSequence / IsInTest
// =============================================================================

TEST_F(OrchestratorTestFixture, IsInSequence_FalseInitially)
{
    lua.script("Context.orchestrator.scope = nil");
    EXPECT_FALSE(lua.script("return Orchestrator.IsInSequence()").get<bool>());
}

TEST_F(OrchestratorTestFixture, IsInSequence_TrueWhenInSequence)
{
    createSequence("Seq", true);
    EXPECT_TRUE(lua.script("return Orchestrator.IsInSequence()").get<bool>());
}

TEST_F(OrchestratorTestFixture, IsInTest_FalseWhenOnlyInSequence)
{
    createSequence("Seq", true);
    EXPECT_FALSE(lua.script("return Orchestrator.IsInTest()").get<bool>());
}

TEST_F(OrchestratorTestFixture, IsInTest_TrueWhenInTest)
{
    createTest("Seq", "T1", true);
    EXPECT_TRUE(lua.script("return Orchestrator.IsInTest()").get<bool>());
}

TEST_F(OrchestratorTestFixture, IsInTest_FalseInitially)
{
    lua.script("Context.orchestrator.scope = nil");
    EXPECT_FALSE(lua.script("return Orchestrator.IsInTest()").get<bool>());
}

// =============================================================================
// HasSequence / HasTest / HasScope
// =============================================================================

TEST_F(OrchestratorTestFixture, HasSequence_FalseForNonExistent)
{
    EXPECT_FALSE(lua.script(
        "return Orchestrator.HasSequence(require('lua/core/framework/scope'):New('NoSuchSeq'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasSequence_TrueAfterCreation)
{
    createSequence("Seq");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasSequence(require('lua/core/framework/scope'):New('Seq'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasTest_FalseForNonExistent)
{
    createSequence("Seq");
    EXPECT_FALSE(lua.script(
        "return Orchestrator.HasTest(require('lua/core/framework/scope'):New('Seq', 'NoSuchTest'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasTest_TrueAfterCreation)
{
    createTest("Seq", "T1");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasTest(require('lua/core/framework/scope'):New('Seq', 'T1'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasScope_SequenceOnly)
{
    createSequence("Seq");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasScope(require('lua/core/framework/scope'):New('Seq'))").get<bool>());
    EXPECT_FALSE(lua.script(
        "return Orchestrator.HasScope(require('lua/core/framework/scope'):New('Nope'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasScope_SequenceAndTest)
{
    createTest("Seq", "T1");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasScope(require('lua/core/framework/scope'):New('Seq', 'T1'))").get<bool>());
    EXPECT_FALSE(lua.script(
        "return Orchestrator.HasScope(require('lua/core/framework/scope'):New('Seq', 'T2'))").get<bool>());
}

// =============================================================================
// SetValue / GetValue / HasValue
// =============================================================================

TEST_F(OrchestratorTestFixture, SetAndGetValue)
{
    createTest("Seq", "T1", true);
    lua.script(R"(
        local scope = Orchestrator.GetScope()
        Orchestrator.SetValue(scope, "voltage", 3.3)
    )");
    auto val = lua.script(R"(
        local scope = Orchestrator.GetScope()
        return Orchestrator.GetValue(scope, "voltage")
    )").get<double>();
    EXPECT_DOUBLE_EQ(val, 3.3);
}

TEST_F(OrchestratorTestFixture, HasValue_TrueAfterSet)
{
    createTest("Seq", "T1", true);
    lua.script(R"(
        local scope = Orchestrator.GetScope()
        Orchestrator.SetValue(scope, "x", 42)
    )");
    EXPECT_TRUE(lua.script(R"(
        local scope = Orchestrator.GetScope()
        return Orchestrator.HasValue(scope, "x")
    )").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasValue_FalseForNonExistent)
{
    createTest("Seq", "T1", true);
    EXPECT_FALSE(lua.script(R"(
        local scope = Orchestrator.GetScope()
        return Orchestrator.HasValue(scope, "nonexistent")
    )").get<bool>());
}

TEST_F(OrchestratorTestFixture, GetValue_ThrowsForNonExistent)
{
    createTest("Seq", "T1", true);
    sol::protected_function_result result = lua.safe_script(R"(
        local scope = Orchestrator.GetScope()
        Orchestrator.GetValue(scope, "nonexistent")
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(OrchestratorTestFixture, SetValue_DuplicateThrowsInExecution)
{
    createTest("Seq", "T1", true);
    setStage("execution");
    lua.script(R"(
        local scope = Orchestrator.GetScope()
        Orchestrator.SetValue(scope, "x", 1)
    )");
    sol::protected_function_result result = lua.safe_script(R"(
        local scope = Orchestrator.GetScope()
        Orchestrator.SetValue(scope, "x", 2)
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(OrchestratorTestFixture, SetValue_DuplicateAllowedInGeneration)
{
    createTest("Seq", "T1", true);
    // Stage is already generation (default)
    lua.script(R"(
        local scope = Orchestrator.GetScope()
        Orchestrator.SetValue(scope, "x", 1)
        Orchestrator.SetValue(scope, "x", 2)
    )");
    auto val = lua.script(R"(
        local scope = Orchestrator.GetScope()
        return Orchestrator.GetValue(scope, "x")
    )").get<int>();
    EXPECT_EQ(val, 2);
}

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
