#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

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
