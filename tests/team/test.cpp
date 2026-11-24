#include "orchestrator_test_fixture.h"
#include <gtest/gtest.h>

// =============================================================================
// Team.Sync — real team.lua, host side mocked through Team.__sync
// =============================================================================

class TeamSyncTest : public OrchestratorTestFixture
{
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        lua.script_file(getLuaBaseDir() + "/lua/core/sdk/environment/team.lua");
        lua.script(R"(
            ScopeResult = require('lua/core/framework/scope_result')
            Context.team.hasTeam = true
            sent = nil
            teammates = Team.Status.pass
            function Team.__sync(status)
                sent = status
                if teammates == nil then return status end
                return teammates
            end
            function Finished(value)
                local r = ScopeResult:New()
                r:Set(value)
                r.time.stop = 1
                return r
            end
        )");
    }

    int sent() { return lua.script("return sent").get<int>(); }
};

TEST_F(TeamSyncTest, NoTeam_DoesNothing)
{
    lua.script("Context.team.hasTeam = false; r = Finished('fail'); Team.Sync(r)");
    EXPECT_TRUE(lua.script("return sent == nil").get<bool>());
    EXPECT_EQ(lua.script("return r.result").get<std::string>(), "fail");
}

TEST_F(TeamSyncTest, Pass_SendsPass)
{
    lua.script("Team.Sync(Finished('pass'))");
    EXPECT_EQ(sent(), 0);
}

TEST_F(TeamSyncTest, NotPassed_SendsFail)
{
    lua.script("teammates = nil");
    for (const char* value : {"fail", "skipped", "disabled"}) {
        lua.script(std::string("r = Finished('") + value + "'); Team.Sync(r)");
        EXPECT_EQ(sent(), 1) << value;
        EXPECT_EQ(lua.script("return r.result").get<std::string>(), value);
    }
}

TEST_F(TeamSyncTest, Unfinished_SendsCriticalFailure)
{
    lua.script("teammates = nil; r = ScopeResult:New(); Team.Sync(r)");
    EXPECT_EQ(sent(), 2);
}

TEST_F(TeamSyncTest, TeammateFailure_FailsResult)
{
    lua.script("teammates = Team.Status.fail; r = Finished('pass'); Team.Sync(r)");
    EXPECT_EQ(lua.script("return r.result").get<std::string>(), "fail");
    EXPECT_EQ(lua.script("return r.reason").get<std::string>(), "Teammate failure");
    EXPECT_FALSE(lua.script("return r.pass").get<bool>());
}

TEST_F(TeamSyncTest, TeammateCriticalFailure_FailsResultAndThrows)
{
    lua.script("teammates = Team.Status.critical_failure; r = Finished('pass')");
    auto result = lua.safe_script("Team.Sync(r)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
    EXPECT_EQ(lua.script("return r.result").get<std::string>(), "fail");
    EXPECT_EQ(lua.script("return r.reason").get<std::string>(), "Teammate critical failure");
}

TEST_F(TeamSyncTest, SameStatus_LeavesResult)
{
    lua.script("teammates = Team.Status.fail; r = Finished('skipped'); Team.Sync(r)");
    EXPECT_EQ(lua.script("return r.result").get<std::string>(), "skipped");
}
