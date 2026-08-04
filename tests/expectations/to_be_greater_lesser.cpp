#include "lua_test_fixture.h"

#include <gtest/gtest.h>

class ExpectationsTest : public LuaTestFixture
{
protected:
    void SetUp() override
    {
        LuaTestFixture::SetUp();
        // Context.info.stage is already Stage.execution from fixture
        // Load the common expectation functions (they define globals)
        lua.script_file(getLuaBaseDir() + "/lua/core/framework/expectation/utils.lua");
    }
};

// =============================================================================
// ExpectToBeGreater / ExpectToBeGreaterOrEqual
// =============================================================================

TEST_F(ExpectationsTest, ToBeGreater_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeGreater(11, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeGreater_FailsAtBoundary)
{
    EXPECT_FALSE(lua.script("return ExpectToBeGreater(10, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeGreater_FailsBelow)
{
    EXPECT_FALSE(lua.script("return ExpectToBeGreater(9, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeGreaterOrEqual_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeGreaterOrEqual(10, 10).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeGreaterOrEqual(11, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeGreaterOrEqual_Fails)
{
    EXPECT_FALSE(lua.script("return ExpectToBeGreaterOrEqual(9, 10).pass").get<bool>());
}

// =============================================================================
// ExpectToBeLesser / ExpectToBeLesserOrEqual
// =============================================================================

TEST_F(ExpectationsTest, ToBeLesser_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeLesser(9, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeLesser_FailsAtBoundary)
{
    EXPECT_FALSE(lua.script("return ExpectToBeLesser(10, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeLesser_FailsAbove)
{
    EXPECT_FALSE(lua.script("return ExpectToBeLesser(11, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeLesserOrEqual_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeLesserOrEqual(10, 10).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeLesserOrEqual(9, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeLesserOrEqual_Fails)
{
    EXPECT_FALSE(lua.script("return ExpectToBeLesserOrEqual(11, 10).pass").get<bool>());
}
