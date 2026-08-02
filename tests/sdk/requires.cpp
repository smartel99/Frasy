#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class SdkRequiresTest : public OrchestratorTestFixture {};

TEST_F(SdkRequiresTest, Requires_MetRequirementPasses)
{
    lua.script(R"(
        Sequence("Seq", function()
            Test("T1", function()
                Requires(RequirementSpecifier(function() return true end))
            end)
        end)
    )");
    // No throw during generation
    SUCCEED();
}

TEST_F(SdkRequiresTest, Requires_UnmetRequirementThrows)
{
    createTest("Seq", "T1", true);
    setStage("execution");
    lua.script("RuntimeRequirement = require('lua/core/framework/runtime_requirement')");

    sol::protected_function_result result = lua.safe_script(R"(
        local req = RuntimeRequirement:New(function() return false end, "test failed")
        Requires(req)
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(SdkRequiresTest, Requires_UnmetThrowsWithReasonInWhat)
{
    createTest("Seq", "T1", true);
    setStage("execution");
    lua.script("RuntimeRequirement = require('lua/core/framework/runtime_requirement')");

    // Capture the error as a Lua table to inspect its structure
    lua.script(R"(
        __err = nil
        local ok, err = pcall(function()
            local req = RuntimeRequirement:New(function() return false end, "voltage too low")
            Requires(req)
        end)
        if not ok then __err = err end
    )");
    auto what = lua.script("return __err.what").get<std::string>();
    EXPECT_NE(what.find("voltage too low"), std::string::npos);
}

TEST_F(SdkRequiresTest, RequirementSpecifier_CreatesRuntimeRequirement)
{
    auto isMet = lua.script(R"(
        local req = RequirementSpecifier(function() return true end)
        return req:IsMet()
    )").get<bool>();
    EXPECT_TRUE(isMet);
}
