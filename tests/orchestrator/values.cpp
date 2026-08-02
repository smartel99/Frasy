#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

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
