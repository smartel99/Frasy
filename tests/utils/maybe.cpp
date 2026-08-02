#include "lua_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// maybe module
// =============================================================================

class MaybeTest : public LuaTestFixture
{
protected:
    void SetUp() override
    {
        LuaTestFixture::SetUp();
        lua.script("Is = require('lua/core/utils/is')");
        lua.script("Maybe = require('lua/core/utils/maybe')");
    }
};

TEST_F(MaybeTest, NilAlwaysPasses)
{
    EXPECT_TRUE(lua.script("return Maybe(nil, Is.Integer)").get<bool>());
    EXPECT_TRUE(lua.script("return Maybe(nil, Is.String)").get<bool>());
    EXPECT_TRUE(lua.script("return Maybe(nil, Is.Table)").get<bool>());
}

TEST_F(MaybeTest, ValidValuePasses)
{
    EXPECT_TRUE(lua.script("return Maybe(5, Is.Integer)").get<bool>());
    EXPECT_TRUE(lua.script("return Maybe('hi', Is.String)").get<bool>());
}

TEST_F(MaybeTest, InvalidValueFails)
{
    EXPECT_FALSE(lua.script("return Maybe('hi', Is.Integer)").get<bool>());
    EXPECT_FALSE(lua.script("return Maybe(5, Is.String)").get<bool>());
}

TEST_F(MaybeTest, WithAdditionalArgs)
{
    EXPECT_TRUE(lua.script("return Maybe(5, Is.IntegerIn, 0, 10)").get<bool>());
    EXPECT_FALSE(lua.script("return Maybe(15, Is.IntegerIn, 0, 10)").get<bool>());
    EXPECT_TRUE(lua.script("return Maybe(nil, Is.IntegerIn, 0, 10)").get<bool>());
}
