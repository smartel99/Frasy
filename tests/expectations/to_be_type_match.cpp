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
// ExpectToBeType
// =============================================================================

TEST_F(ExpectationsTest, ToBeType_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeType(42, 'number').pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeType('hi', 'string').pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeType({}, 'table').pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeType(true, 'boolean').pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeType_Fails)
{
    EXPECT_FALSE(lua.script("return ExpectToBeType(42, 'string').pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeType('hi', 'number').pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeType_Fields)
{
    lua.script("r = ExpectToBeType(42, 'string')");
    EXPECT_EQ(lua.script("return r.method").get<std::string>(), "ToBeType");
    EXPECT_EQ(lua.script("return r.expected").get<std::string>(), "string");
    EXPECT_EQ(lua.script("return r.type").get<std::string>(), "number");
}

// =============================================================================
// ExpectToBeMatch
// =============================================================================

TEST_F(ExpectationsTest, ToMatch_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeMatch('hello world', 'hello').pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeMatch('test123', '%d+').pass").get<bool>());
}

TEST_F(ExpectationsTest, ToMatch_Fails)
{
    EXPECT_FALSE(lua.script("return ExpectToBeMatch('hello', 'xyz').pass").get<bool>());
}

TEST_F(ExpectationsTest, ToMatch_NonStringFails)
{
    EXPECT_FALSE(lua.script("return ExpectToBeMatch(123, '%d+').pass").get<bool>());
}
