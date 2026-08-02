#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// RuntimeRequirement
// =============================================================================

class RuntimeRequirementTest : public OrchestratorTestFixture
{
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        lua.script("RuntimeRequirement = require('lua/core/framework/runtime_requirement')");
    }
};

TEST_F(RuntimeRequirementTest, IsMet_ReturnsTrue)
{
    auto result = lua.script(R"(
        local req = RuntimeRequirement:New(function() return true end)
        return req:IsMet()
    )").get<bool>();
    EXPECT_TRUE(result);
}

TEST_F(RuntimeRequirementTest, IsMet_ReturnsFalse)
{
    auto result = lua.script(R"(
        local req = RuntimeRequirement:New(function() return false end)
        return req:IsMet()
    )").get<bool>();
    EXPECT_FALSE(result);
}

TEST_F(RuntimeRequirementTest, IsMet_NonBooleanThrows)
{
    sol::protected_function_result result = lua.safe_script(R"(
        local req = RuntimeRequirement:New(function() return "yes" end)
        req:IsMet()
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(RuntimeRequirementTest, IsMet_NilReturnThrows)
{
    sol::protected_function_result result = lua.safe_script(R"(
        local req = RuntimeRequirement:New(function() return nil end)
        req:IsMet()
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(RuntimeRequirementTest, ReasonAccessible)
{
    auto reason = lua.script(R"(
        local req = RuntimeRequirement:New(function() return false end, "custom reason")
        return req.reason
    )").get<std::string>();
    EXPECT_EQ(reason, "custom reason");
}

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

// =============================================================================
// ScopeRequirement — Execution stage
// =============================================================================

class ScopeRequirementExecutionTest : public OrchestratorTestFixture
{
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        // Create sequences and tests using low-level API (not SDK)
        // so they actually exist without needing Generate()
        createTest("SeqA", "T1");
        createTest("SeqB", "T1");
        // Switch to execution stage
        setStage("execution");
        // Set scope to SeqB.T1
        lua.script("Context.orchestrator.scope = require('lua/core/framework/scope'):New('SeqB', 'T1')");
        // Clear module cache so execution variant is loaded
        lua.script("package.loaded['lua/core/framework/scope_requirement/module'] = nil");
    }
};

TEST_F(ScopeRequirementExecutionTest, ToPass_ChecksHasPassed)
{
    // SeqA has not passed yet (default pass=false)
    lua.script("local ScopeReq = require('lua/core/framework/scope_requirement/execution')");
    auto isMet = lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/execution')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        local rt = req:ToPass()
        return rt:IsMet()
    )").get<bool>();
    EXPECT_FALSE(isMet);

    // Mark SeqA as passed
    lua.script("Context.orchestrator.sequences['SeqA'].result.pass = true");
    auto isMetNow = lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/execution')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        local rt = req:ToPass()
        return rt:IsMet()
    )").get<bool>();
    EXPECT_TRUE(isMetNow);
}

TEST_F(ScopeRequirementExecutionTest, ToFail_ChecksNotPassed)
{
    // SeqA has not passed (default)
    auto isMet = lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/execution')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        local rt = req:ToFail()
        return rt:IsMet()
    )").get<bool>();
    EXPECT_TRUE(isMet); // not passed = "failed"

    // Mark SeqA as passed
    lua.script("Context.orchestrator.sequences['SeqA'].result.pass = true");
    auto isMetNow = lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/execution')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        local rt = req:ToFail()
        return rt:IsMet()
    )").get<bool>();
    EXPECT_FALSE(isMetNow);
}

TEST_F(ScopeRequirementExecutionTest, ToBeComplete_ChecksNotSkipped)
{
    auto isMet = lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/execution')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        local rt = req:ToBeComplete()
        return rt:IsMet()
    )").get<bool>();
    EXPECT_TRUE(isMet); // not skipped by default

    lua.script("Context.orchestrator.sequences['SeqA'].result.skipped = true");
    auto isMetNow = lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/execution')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        local rt = req:ToBeComplete()
        return rt:IsMet()
    )").get<bool>();
    EXPECT_FALSE(isMetNow);
}

TEST_F(ScopeRequirementExecutionTest, OrderMethods_ReturnAlwaysTrue)
{
    // In execution, ordering is already resolved — these always return true
    EXPECT_TRUE(lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/execution')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        return req:ToBeFirst():IsMet()
    )").get<bool>());

    EXPECT_TRUE(lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/execution')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        return req:ToBeLast():IsMet()
    )").get<bool>());
}

TEST_F(ScopeRequirementExecutionTest, HasPassed_ReflectsActualResult)
{
    EXPECT_FALSE(lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/execution')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        return req:HasPassed()
    )").get<bool>());

    lua.script("Context.orchestrator.sequences['SeqA'].result.pass = true");
    EXPECT_TRUE(lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/execution')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        return req:HasPassed()
    )").get<bool>());
}

TEST_F(ScopeRequirementExecutionTest, Value_RetrievesExportedValue)
{
    // Set a value for SeqA.T1
    lua.script(R"(
        Context.orchestrator.values['SeqA']['T1']['voltage'] = 3.3
    )");
    auto val = lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/execution')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA', 'T1'))
        return req:Value('voltage')
    )").get<double>();
    EXPECT_DOUBLE_EQ(val, 3.3);
}

TEST_F(ScopeRequirementExecutionTest, Test_SetsTestOnScope)
{
    // Should not throw for existing test
    lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/execution')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        req:Test('T1')
    )");
    SUCCEED();
}

TEST_F(ScopeRequirementExecutionTest, Test_NonExistentThrows)
{
    sol::protected_function_result result = lua.safe_script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/execution')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        req:Test('NoSuchTest')
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

// =============================================================================
// ScopeRequirement — Validation stage
// =============================================================================

class ScopeRequirementValidationTest : public OrchestratorTestFixture
{
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        createTest("SeqA", "T1");
        setStage("validation");
        lua.script("Context.orchestrator.scope = require('lua/core/framework/scope'):New('SeqA', 'T1')");
        lua.script("package.loaded['lua/core/framework/scope_requirement/module'] = nil");
    }
};

TEST_F(ScopeRequirementValidationTest, AllMethodsReturnTrueRequirements)
{
    // In validation, everything is permissive
    EXPECT_TRUE(lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/validation')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        return req:ToBeFirst():IsMet()
    )").get<bool>());

    EXPECT_TRUE(lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/validation')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        return req:ToBeLast():IsMet()
    )").get<bool>());

    EXPECT_TRUE(lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/validation')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        return req:ToPass():IsMet()
    )").get<bool>());

    EXPECT_TRUE(lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/validation')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        return req:ToFail():IsMet()
    )").get<bool>());

    EXPECT_TRUE(lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/validation')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        return req:ToBeComplete():IsMet()
    )").get<bool>());
}

TEST_F(ScopeRequirementValidationTest, HasPassed_ReturnsTrueAlways)
{
    EXPECT_TRUE(lua.script(R"(
        local ScopeReq = require('lua/core/framework/scope_requirement/validation')
        local req = ScopeReq:New(Orchestrator, require('lua/core/framework/scope'):New('SeqA'))
        return req:HasPassed()
    )").get<bool>());
}

// =============================================================================
// OrderRequirement
// =============================================================================

TEST_F(OrchestratorTestFixture, OrderRequirement_New_StoresFields)
{
    lua.script(R"(
        local OR = require('lua/core/framework/order_requirement')
        local Scope = require('lua/core/framework/scope')
        local s = Scope:New('SeqA')
        local r = Scope:New('SeqB')
        order_req = OR:New(s, r, OR.Kind.after)
    )");
    auto kind = lua.script("return order_req.kind").get<int>();
    EXPECT_EQ(kind, 3); // after
    auto seqName = lua.script("return order_req.scope.sequence").get<std::string>();
    EXPECT_EQ(seqName, "SeqA");
    auto refName = lua.script("return order_req.reference.sequence").get<std::string>();
    EXPECT_EQ(refName, "SeqB");
}

TEST_F(OrchestratorTestFixture, OrderRequirement_ToString)
{
    lua.script(R"(
        local OR = require('lua/core/framework/order_requirement')
        local Scope = require('lua/core/framework/scope')
        order_req = OR:New(Scope:New('SeqA', 'T1'), Scope:New('SeqB'), OR.Kind.first)
    )");
    auto str = lua.script("return tostring(order_req)").get<std::string>();
    EXPECT_NE(str.find("First"), std::string::npos);
    EXPECT_NE(str.find("SeqA"), std::string::npos);
}

// =============================================================================
// SyncRequirement
// =============================================================================

TEST_F(OrchestratorTestFixture, SyncRequirement_New_GlobalKind)
{
    lua.script(R"(
        local SR = require('lua/core/framework/sync_requirement')
        local Scope = require('lua/core/framework/scope')
        sync_req = SR:New(Scope:New('SeqA', 'T1'))
    )");
    auto kind = lua.script("return sync_req.kind").get<int>();
    EXPECT_EQ(kind, 1); // global
}

TEST_F(OrchestratorTestFixture, SyncRequirement_Ib_ChangesKind)
{
    lua.script(R"(
        local SR = require('lua/core/framework/sync_requirement')
        local Scope = require('lua/core/framework/scope')
        sync_req = SR:New(Scope:New('SeqA'))
        sync_req:Ib()
    )");
    auto kind = lua.script("return sync_req.kind").get<int>();
    EXPECT_EQ(kind, 2); // ib
}

TEST_F(OrchestratorTestFixture, SyncRequirement_IsMet_AlwaysTrue)
{
    auto result = lua.script(R"(
        local SR = require('lua/core/framework/sync_requirement')
        local Scope = require('lua/core/framework/scope')
        local req = SR:New(Scope:New('SeqA'))
        return req:IsMet()
    )").get<bool>();
    EXPECT_TRUE(result);
}
