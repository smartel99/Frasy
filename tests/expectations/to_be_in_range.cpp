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
        lua.script_file(getLuaBaseDir() + "/lua/core/framework/expectation/common.lua");
    }
};

// =============================================================================
// ExpectToBeInRange
// =============================================================================

TEST_F(ExpectationsTest, ToBeInRange_Within)
{
    EXPECT_TRUE(lua.script("return ExpectToBeInRange(5, 0, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeInRange_AtBoundary)
{
    EXPECT_TRUE(lua.script("return ExpectToBeInRange(0, 0, 10).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeInRange(10, 0, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeInRange_Outside)
{
    EXPECT_FALSE(lua.script("return ExpectToBeInRange(-1, 0, 10).pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeInRange(11, 0, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeInRange_NonNumber)
{
    EXPECT_FALSE(lua.script("return ExpectToBeInRange('5', 0, 10).pass").get<bool>());
}

// =============================================================================
// ExpectToBeInPercentage
// =============================================================================

TEST_F(ExpectationsTest, ToBeInPercentage_Within)
{
    // 3.3 ± 5% -> [3.135, 3.465]
    EXPECT_TRUE(lua.script("return ExpectToBeInPercentage(3.3, 3.3, 5.0).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeInPercentage(3.4, 3.3, 5.0).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeInPercentage_Outside)
{
    EXPECT_FALSE(lua.script("return ExpectToBeInPercentage(3.5, 3.3, 5.0).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeInPercentage_Fields)
{
    lua.script("r = ExpectToBeInPercentage(3.3, 3.3, 5.0)");
    EXPECT_EQ(lua.script("return r.method").get<std::string>(), "ToBeInPercentage");
    EXPECT_DOUBLE_EQ(lua.script("return r.percentage").get<double>(), 5.0);
    // deviation = |3.3 * 5 / 100| = 0.165
    EXPECT_NEAR(lua.script("return r.deviation").get<double>(), 0.165, 1e-10);
    EXPECT_NEAR(lua.script("return r.min").get<double>(), 3.135, 1e-10);
    EXPECT_NEAR(lua.script("return r.max").get<double>(), 3.465, 1e-10);
}
