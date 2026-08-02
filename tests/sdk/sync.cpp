#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class SdkSyncTest : public OrchestratorTestFixture {};

TEST_F(SdkSyncTest, Sync_ReturnsSyncRequirement)
{
    lua.script(R"(
        __req_type = nil
        __is_met = nil
        Sequence("Seq", function()
            Test("T1", function()
                local req = Sync()
                __req_type = type(req)
                __is_met = req:IsMet()
            end)
        end)
    )");
    lua.script("Orchestrator.Generate()");
    EXPECT_EQ(lua.script("return __req_type").get<std::string>(), "table");
    EXPECT_TRUE(lua.script("return __is_met").get<bool>());
}

TEST_F(SdkSyncTest, Sync_RegistersWithOrchestrator)
{
    lua.script(R"(
        Sequence("Seq", function()
            Test("T1", function()
                Sync()
            end)
        end)
    )");
    lua.script("Orchestrator.Generate()");
    auto count = lua.script("return #Context.orchestrator.sync_requirements").get<int>();
    EXPECT_EQ(count, 1);
}

TEST_F(SdkSyncTest, Sync_OutsideSequenceThrows)
{
    lua.script("Context.orchestrator.scope = nil");
    sol::protected_function_result result = lua.safe_script("Sync()", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(SdkSyncTest, Sync_MultipleCalls_RegistersMultiple)
{
    lua.script(R"(
        Sequence("Seq", function()
            Test("T1", function()
                Sync()
            end)
            Test("T2", function()
                Sync()
            end)
        end)
    )");
    lua.script("Orchestrator.Generate()");
    auto count = lua.script("return #Context.orchestrator.sync_requirements").get<int>();
    EXPECT_EQ(count, 2);
}
