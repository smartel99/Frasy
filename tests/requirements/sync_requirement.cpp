#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

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
