#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// ScopeResult
// =============================================================================

class ScopeResultTest : public OrchestratorTestFixture {
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        lua.script("ScopeResult = require('lua/core/framework/scope_result')");
    }

    std::string aggregate(const std::string& children)
    {
        return lua.script("return ScopeResult.Aggregate(" + children + ")").get<std::string>();
    }
};

TEST_F(ScopeResultTest, New_DefaultsToFail)
{
    lua.script("r = ScopeResult:New()");
    EXPECT_EQ(lua.script("return r.result").get<std::string>(), "fail");
    EXPECT_FALSE(lua.script("return r.pass").get<bool>());
    EXPECT_FALSE(lua.script("return r.skipped").get<bool>());
    EXPECT_TRUE(lua.script("return r.enabled").get<bool>());
    EXPECT_TRUE(lua.script("return r.reason == nil").get<bool>());
    EXPECT_EQ(lua.script("return r.time.stop").get<int>(), 0);
}

TEST_F(ScopeResultTest, New_InstancesDoNotShareTime)
{
    lua.script("a = ScopeResult:New(); b = ScopeResult:New(); a.time.stop = 5");
    EXPECT_EQ(lua.script("return b.time.stop").get<int>(), 0);
}

struct DeprecatedFields {
    const char* value;
    bool        pass;
    bool        skipped;
    bool        enabled;
};

class ScopeResultSetTest : public ScopeResultTest, public ::testing::WithParamInterface<DeprecatedFields> {};

TEST_P(ScopeResultSetTest, Set_DerivesDeprecatedFields)
{
    const auto& p = GetParam();
    lua.script(std::string("r = ScopeResult:New(); r:Set('") + p.value + "', 'why')");
    EXPECT_EQ(lua.script("return r.result").get<std::string>(), p.value);
    EXPECT_EQ(lua.script("return r.reason").get<std::string>(), "why");
    EXPECT_EQ(lua.script("return r.pass").get<bool>(), p.pass);
    EXPECT_EQ(lua.script("return r.skipped").get<bool>(), p.skipped);
    EXPECT_EQ(lua.script("return r.enabled").get<bool>(), p.enabled);
}

INSTANTIATE_TEST_SUITE_P(AllValues,
                         ScopeResultSetTest,
                         ::testing::Values(DeprecatedFields {"pass", true, false, true},
                                           DeprecatedFields {"fail", false, false, true},
                                           DeprecatedFields {"skipped", false, true, true},
                                           DeprecatedFields {"disabled", false, true, false},
                                           DeprecatedFields {"incomplete", false, false, true}),
                         [](const auto& info) { return std::string(info.param.value); });

TEST_F(ScopeResultTest, Set_WithoutReasonClearsIt)
{
    lua.script("r = ScopeResult:New(); r:Set('fail', 'why'); r:Set('pass')");
    EXPECT_TRUE(lua.script("return r.reason == nil").get<bool>());
}

TEST_F(ScopeResultTest, Set_RejectsUnknownValue)
{
    auto result = lua.safe_script("ScopeResult:New():Set('passed')", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(ScopeResultTest, Set_RejectsNonStringReason)
{
    auto result = lua.safe_script("ScopeResult:New():Set('fail', { what = 'x' })", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(ScopeResultTest, Set_RejectedValueLeavesResultUntouched)
{
    lua.script("r = ScopeResult:New(); r:Set('pass')");
    lua.safe_script("r:Set('bogus')", sol::script_pass_on_error);
    EXPECT_EQ(lua.script("return r.result").get<std::string>(), "pass");
    EXPECT_TRUE(lua.script("return r.pass").get<bool>());
}

TEST_F(ScopeResultTest, HasPassed_OnlyForPass)
{
    for (const char* value : {"fail", "skipped", "disabled", "incomplete"}) {
        lua.script(std::string("r = ScopeResult:New(); r:Set('") + value + "')");
        EXPECT_FALSE(lua.script("return r:HasPassed()").get<bool>()) << value;
    }
    lua.script("r:Set('pass')");
    EXPECT_TRUE(lua.script("return r:HasPassed()").get<bool>());
}

TEST_F(ScopeResultTest, IsSkipped_ForSkippedAndDisabled)
{
    for (const char* value : {"skipped", "disabled"}) {
        lua.script(std::string("r = ScopeResult:New(); r:Set('") + value + "')");
        EXPECT_TRUE(lua.script("return r:IsSkipped()").get<bool>()) << value;
    }
    for (const char* value : {"pass", "fail", "incomplete"}) {
        lua.script(std::string("r = ScopeResult:New(); r:Set('") + value + "')");
        EXPECT_FALSE(lua.script("return r:IsSkipped()").get<bool>()) << value;
    }
}

TEST_F(ScopeResultTest, Aggregate_EmptyIsPass)
{
    EXPECT_EQ(aggregate("{}"), "pass");
}

TEST_F(ScopeResultTest, Aggregate_AllPass)
{
    EXPECT_EQ(aggregate("{ 'pass', 'pass' }"), "pass");
}

TEST_F(ScopeResultTest, Aggregate_SkippedIsNeutral)
{
    EXPECT_EQ(aggregate("{ 'pass', 'skipped' }"), "pass");
    EXPECT_EQ(aggregate("{ 'skipped', 'skipped' }"), "pass");
}

TEST_F(ScopeResultTest, Aggregate_DisabledMakesIncomplete)
{
    EXPECT_EQ(aggregate("{ 'pass', 'disabled' }"), "incomplete");
    EXPECT_EQ(aggregate("{ 'disabled' }"), "incomplete");
}

TEST_F(ScopeResultTest, Aggregate_IncompleteChildMakesIncomplete)
{
    EXPECT_EQ(aggregate("{ 'pass', 'incomplete', 'skipped' }"), "incomplete");
}

TEST_F(ScopeResultTest, Aggregate_FailWinsInAnyOrder)
{
    EXPECT_EQ(aggregate("{ 'fail', 'disabled' }"), "fail");
    EXPECT_EQ(aggregate("{ 'disabled', 'fail' }"), "fail");
    EXPECT_EQ(aggregate("{ 'incomplete', 'skipped', 'fail', 'pass' }"), "fail");
}
