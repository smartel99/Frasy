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
// ExpectToBeEqual
// =============================================================================

TEST_F(ExpectationsTest, ToBeEqual_SameValue)
{
    EXPECT_TRUE(lua.script("return ExpectToBeEqual(42, 42).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeEqual('hello', 'hello').pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeEqual(true, true).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeEqual_DifferentValue)
{
    EXPECT_FALSE(lua.script("return ExpectToBeEqual(42, 43).pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeEqual('hello', 'world').pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeEqual_DifferentType)
{
    EXPECT_FALSE(lua.script("return ExpectToBeEqual(42, '42').pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeEqual(1, true).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeEqual_Method)
{
    auto method = lua.script("return ExpectToBeEqual(1, 1).method").get<std::string>();
    EXPECT_EQ(method, "ToBeEqual");
}

TEST_F(ExpectationsTest, ToBeEqual_Table)
{
    EXPECT_TRUE(lua.script("return ExpectToBeEqual({42}, {42}).pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeEqual({42}, {69}).pass").get<bool>());
}
