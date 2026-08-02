#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

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
