#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class SdkTestFuncTest : public OrchestratorTestFixture {};

TEST_F(SdkTestFuncTest, Test_CreatesAndRegisters)
{
    lua.script(R"(
        Sequence("Seq", function()
            Test("CheckVoltage", function() end)
        end)
    )");
    lua.script("Orchestrator.Generate()");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasTest(require('lua/core/framework/scope'):New('Seq', 'CheckVoltage'))").get<bool>());
}

TEST_F(SdkTestFuncTest, Test_OutsideSequenceThrows)
{
    sol::protected_function_result result = lua.safe_script(R"(
        Test("Orphan", function() end)
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(SdkTestFuncTest, Test_GetterReturnsRequirement)
{
    lua.script(R"(
        __req_type = nil
        Sequence("Seq", function()
            Test("T1", function() end)
            Test("T2", function()
                local req = Test("T1")
                __req_type = type(req)
            end)
        end)
    )");
    lua.script("Orchestrator.Generate()");
    auto reqType = lua.script("return __req_type").get<std::string>();
    EXPECT_EQ(reqType, "table");
}

TEST_F(SdkTestFuncTest, Test_GetterWithoutName_ReturnsSelf)
{
    lua.script(R"(
        __req_type = nil
        Sequence("Seq", function()
            Test("T1", function()
                local req = Test()
                __req_type = type(req)
            end)
        end)
    )");
    lua.script("Orchestrator.Generate()");
    auto reqType = lua.script("return __req_type").get<std::string>();
    EXPECT_EQ(reqType, "table");
}

TEST_F(SdkTestFuncTest, Test_MultipleTestsInSequence)
{
    lua.script(R"(
        Sequence("Seq", function()
            Test("T1", function() end)
            Test("T2", function() end)
            Test("T3", function() end)
        end)
    )");
    lua.script("Orchestrator.Generate()");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasTest(require('lua/core/framework/scope'):New('Seq', 'T1'))").get<bool>());
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasTest(require('lua/core/framework/scope'):New('Seq', 'T2'))").get<bool>());
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasTest(require('lua/core/framework/scope'):New('Seq', 'T3'))").get<bool>());
}
