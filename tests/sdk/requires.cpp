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
    )",
                                                            sol::script_pass_on_error);
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
    auto isMet = lua
                   .script(R"(
        local req = RequirementSpecifier(function() return true end)
        return req:IsMet()
    )")
                   .get<bool>();
    EXPECT_TRUE(isMet);
}

// =============================================================================
// Skip reasons say which requirement was unmet
// =============================================================================

class SdkRequiresReasonTest : public OrchestratorTestFixture {
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        createTest("Seq", "T1", true);
        setStage("execution");
    }

    /** What `Requires` raised when running @p code from @p chunk. */
    std::string raised(const std::string& code, const std::string& chunk = "test")
    {
        lua.script("function __run() " + code + " end", chunk);
        return lua.script("local ok, err = pcall(__run); assert(not ok); return err.what").get<std::string>();
    }
};

TEST_F(SdkRequiresReasonTest, SpecifierReason_InWhat)
{
    EXPECT_EQ(raised("Requires(RequirementSpecifier(function() return false end, 'fixture lid closed'))"),
              "UnmetRequirement: fixture lid closed");
}

TEST_F(SdkRequiresReasonTest, NoReason_LocationUnderUserDir)
{
    EXPECT_EQ(
      raised("\n\nRequires(RequirementSpecifier(function() return false end))", "@C:/frasy/lua/user/product/test.lua"),
      "UnmetRequirement: requirement at product/test.lua:3");
}

TEST_F(SdkRequiresReasonTest, NoReason_FileNameElsewhere)
{
    EXPECT_EQ(raised("Requires(RequirementSpecifier(function() return false end))", "@C:\\other\\dir\\seq.lua"),
              "UnmetRequirement: requirement at seq.lua:1");
}

TEST_F(SdkRequiresReasonTest, NonStringReason_Errors)
{
    const auto run = lua.safe_script("RequirementSpecifier(function() return true end, 42)", sol::script_pass_on_error);
    EXPECT_FALSE(run.valid());
}

TEST_F(SdkRequiresReasonTest, NoReason_StringChunkUsesShortSource)
{
    EXPECT_EQ(raised("Requires(RequirementSpecifier(function() return false end))", "=inline"),
              "UnmetRequirement: requirement at inline:1");
}

TEST_F(SdkRequiresReasonTest, NoReason_NoLuaLine)
{
    // A tail call and a direct pcall leave a C frame where the caller was.
    EXPECT_EQ(raised("return Requires(RequirementSpecifier(function() return false end))"),
              "UnmetRequirement: requirement (no reason given)");
    const auto what = lua
                        .script(
                          "local ok, err = pcall(Requires, RequirementSpecifier(function() return false end))\n"
                          "return err.what")
                        .get<std::string>();
    EXPECT_EQ(what, "UnmetRequirement: requirement (no reason given)");
}

TEST_F(SdkRequiresReasonTest, RequirementFunction_CalledOnce)
{
    lua.script("calls = 0; pcall(Requires, RequirementSpecifier(function() calls = calls + 1; return false end))");
    EXPECT_EQ(lua.script("return calls").get<int>(), 1);
}
