#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class ExpectationClassTest : public OrchestratorTestFixture {
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        // Create a sequence and test, enter the test scope in execution stage
        createTest("Seq", "T1", true);
        setStage("execution");
        // Load the expectation execution module
        lua.script("Expectation = require('lua/core/framework/expectation/expectation')");
        // Load common expectation functions
        lua.script_file(getLuaBaseDir() + "/lua/core/framework/expectation/utils.lua");
    }
};

TEST_F(ExpectationClassTest, New_CreatesWithValueAndName)
{
    lua.script(R"(
        local e = Expectation:New(42, "voltage")
        __value = e.value
        __name = e.name
    )");
    EXPECT_EQ(lua.script("return __value").get<int>(), 42);
    EXPECT_EQ(lua.script("return __name").get<std::string>(), "voltage");
}

TEST_F(ExpectationClassTest, New_DefaultsInvertedToFalse)
{
    auto inverted = lua
                      .script(R"(
        local e = Expectation:New(1, "x")
        return e.inverted
    )")
                      .get<bool>();
    EXPECT_FALSE(inverted);
}

TEST_F(ExpectationClassTest, New_DefaultsMandatoryToFalse)
{
    std::optional<int> policy = lua
                                  .script(R"(
        local e = Expectation:New(1, "x")
        return e.policy
    )")
                                  .get<std::optional<int>>();
    EXPECT_FALSE(policy.has_value());
}

TEST_F(ExpectationClassTest, New_WithOptNote)
{
    auto note = lua
                  .script(R"(
        local e = Expectation:New(1, "x", { note = "custom note" })
        return e.note
    )")
                  .get<std::string>();
    EXPECT_EQ(note, "custom note");
}

TEST_F(ExpectationClassTest, New_WithOptExtra)
{
    auto hasExtra = lua
                      .script(R"(
        local e = Expectation:New(1, "x", { extra = { key = "val" } })
        return e.extra.key
    )")
                      .get<std::string>();
    EXPECT_EQ(hasExtra, "val");
}
