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
