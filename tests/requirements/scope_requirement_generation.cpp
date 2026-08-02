#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// ScopeRequirement — Generation stage
// =============================================================================

class ScopeRequirementGenerationTest : public OrchestratorTestFixture
{
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        // Stage is already generation by default
        // Create sequences so scopes exist
        lua.script(R"(
            Sequence("SeqA", function()
                Test("T1", function() end)
                Test("T2", function() end)
            end)
            Sequence("SeqB", function()
                Test("T1", function() end)
            end)
        )");
        // Set scope to SeqB (simulating being inside SeqB during generation)
        lua.script("Context.orchestrator.scope = require('lua/core/framework/scope'):New('SeqB')");
    }
};

TEST_F(ScopeRequirementGenerationTest, ToBeFirst_RegistersOrderRequirement)
{
    lua.script(R"(
        local req = Orchestrator.GetSequenceScopeRequirement()
        req:ToBeFirst()
    )");
    auto count = lua.script("return #Context.orchestrator.order_requirements").get<int>();
    EXPECT_EQ(count, 1);
    auto kind = lua.script("return Context.orchestrator.order_requirements[1].kind").get<int>();
    EXPECT_EQ(kind, 1); // OrderRequirement.Kind.first
}

TEST_F(ScopeRequirementGenerationTest, ToBeLast_RegistersOrderRequirement)
{
    lua.script(R"(
        local req = Orchestrator.GetSequenceScopeRequirement()
        req:ToBeLast()
    )");
    auto kind = lua.script("return Context.orchestrator.order_requirements[1].kind").get<int>();
    EXPECT_EQ(kind, 2); // OrderRequirement.Kind.last
}

TEST_F(ScopeRequirementGenerationTest, ToPass_RegistersAfterRequirement)
{
    lua.script(R"(
        local req = Orchestrator.GetSequenceScopeRequirement("SeqA")
        req:ToPass()
    )");
    auto count = lua.script("return #Context.orchestrator.order_requirements").get<int>();
    EXPECT_EQ(count, 1);
    auto kind = lua.script("return Context.orchestrator.order_requirements[1].kind").get<int>();
    EXPECT_EQ(kind, 3); // OrderRequirement.Kind.after
}

TEST_F(ScopeRequirementGenerationTest, ToBeAfter_RegistersAfterRequirement)
{
    lua.script(R"(
        local req = Orchestrator.GetSequenceScopeRequirement("SeqA")
        req:ToBeAfter()
    )");
    auto count = lua.script("return #Context.orchestrator.order_requirements").get<int>();
    EXPECT_EQ(count, 1);
    auto kind = lua.script("return Context.orchestrator.order_requirements[1].kind").get<int>();
    EXPECT_EQ(kind, 3); // after
}

TEST_F(ScopeRequirementGenerationTest, Test_SetsTestField)
{
    lua.script(R"(
        local req = Orchestrator.GetSequenceScopeRequirement("SeqA")
        req:Test("T1")
    )");
    // Shouldn't throw — the test exists
    SUCCEED();
}

TEST_F(ScopeRequirementGenerationTest, HasPassed_ReturnsTrueInGeneration)
{
    auto result = lua.script(R"(
        local req = Orchestrator.GetSequenceScopeRequirement("SeqA")
        return req:HasPassed()
    )").get<bool>();
    EXPECT_TRUE(result);
}

TEST_F(ScopeRequirementGenerationTest, HasFailed_ReturnsTrueInGeneration)
{
    auto result = lua.script(R"(
        local req = Orchestrator.GetSequenceScopeRequirement("SeqA")
        return req:HasFailed()
    )").get<bool>();
    EXPECT_TRUE(result);
}

TEST_F(ScopeRequirementGenerationTest, ToPass_ReturnsRuntimeRequirement)
{
    auto isMet = lua.script(R"(
        local req = Orchestrator.GetSequenceScopeRequirement("SeqA")
        local rt = req:ToPass()
        return rt:IsMet()
    )").get<bool>();
    EXPECT_TRUE(isMet); // Always true in generation
}
