#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

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
