#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class SdkSequenceTest : public OrchestratorTestFixture {};

TEST_F(SdkSequenceTest, Sequence_CreatesAndRegisters)
{
    lua.script(R"(
        Sequence("PowerOn", function()
            Test("T1", function() end)
        end)
    )");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasSequence(require('lua/core/framework/scope'):New('PowerOn'))").get<bool>());
}

TEST_F(SdkSequenceTest, Sequence_DuplicateThrows)
{
    lua.script(R"(
        Sequence("PowerOn", function()
            Test("T1", function() end)
        end)
    )");
    sol::protected_function_result result = lua.safe_script(R"(
        Sequence("PowerOn", function()
            Test("T1", function() end)
        end)
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(SdkSequenceTest, Sequence_GetterReturnsRequirement)
{
    lua.script(R"(
        __req_type = nil
        Sequence("SeqA", function()
            Test("T1", function() end)
        end)
        Sequence("SeqB", function()
            local req = Sequence("SeqA")
            __req_type = type(req)
            Test("T1", function() end)
        end)
    )");
    lua.script("Orchestrator.Generate()");
    auto reqType = lua.script("return __req_type").get<std::string>();
    EXPECT_EQ(reqType, "table");
}

TEST_F(SdkSequenceTest, Sequence_GetterWithoutName_ReturnsSelf)
{
    lua.script(R"(
        __req_type = nil
        Sequence("SeqA", function()
            local req = Sequence()
            __req_type = type(req)
            Test("T1", function() end)
        end)
    )");
    lua.script("Orchestrator.Generate()");
    auto reqType = lua.script("return __req_type").get<std::string>();
    EXPECT_EQ(reqType, "table");
}

TEST_F(SdkSequenceTest, Sequence_GetterOutsideSequenceThrows)
{
    sol::protected_function_result result = lua.safe_script(R"(
        Sequence("SeqA")
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}
