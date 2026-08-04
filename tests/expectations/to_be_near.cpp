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
// ExpectToBeNear
// =============================================================================

TEST_F(ExpectationsTest, ToBeNear_WithinDeviation)
{
    EXPECT_TRUE(lua.script("return ExpectToBeNear(10.5, 10.0, 1.0).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeNear(9.5, 10.0, 1.0).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeNear_AtBoundary)
{
    EXPECT_TRUE(lua.script("return ExpectToBeNear(11.0, 10.0, 1.0).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeNear(9.0, 10.0, 1.0).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeNear_OutsideDeviation)
{
    EXPECT_FALSE(lua.script("return ExpectToBeNear(11.1, 10.0, 1.0).pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeNear(8.9, 10.0, 1.0).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeNear_NonNumber)
{
    EXPECT_FALSE(lua.script("return ExpectToBeNear('10', 10.0, 1.0).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeNear_Fields)
{
    lua.script("r = ExpectToBeNear(5.0, 10.0, 2.0)");
    EXPECT_EQ(lua.script("return r.method").get<std::string>(), "ToBeNear");
    EXPECT_DOUBLE_EQ(lua.script("return r.min").get<double>(), 8.0);
    EXPECT_DOUBLE_EQ(lua.script("return r.max").get<double>(), 12.0);
}
