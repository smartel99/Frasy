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
// ExpectToBeTrue
// =============================================================================

TEST_F(ExpectationsTest, ToBeTrue_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeTrue(true).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeTrue_FailsWithFalse)
{
    EXPECT_FALSE(lua.script("return ExpectToBeTrue(false).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeTrue_FailsWithNonBoolean)
{
    EXPECT_FALSE(lua.script("return ExpectToBeTrue(1).pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeTrue('true').pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeTrue(nil).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeTrue_Method)
{
    auto method = lua.script("return ExpectToBeTrue(true).method").get<std::string>();
    EXPECT_EQ(method, "ToBeTrue");
}

// =============================================================================
// ExpectToBeFalse
// =============================================================================

TEST_F(ExpectationsTest, ToBeFalse_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeFalse(false).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeFalse_FailsWithTrue)
{
    EXPECT_FALSE(lua.script("return ExpectToBeFalse(true).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeFalse_FailsWithNonBoolean)
{
    EXPECT_FALSE(lua.script("return ExpectToBeFalse(0).pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeFalse(nil).pass").get<bool>());
}
