#include "lua_test_fixture.h"

#include <gtest/gtest.h>

class CheckFieldTest : public LuaTestFixture
{
protected:
    void SetUp() override
    {
        LuaTestFixture::SetUp();
        lua.script("Is = require('lua/core/utils/is')");
        lua.script("Maybe = require('lua/core/utils/maybe')");
        lua.script("CheckField = require('lua/core/utils/check_field')");
    }
};

// =============================================================================
// Valid fields with additional predicate arguments
// =============================================================================

TEST_F(CheckFieldTest, ValidIntegerInRange)
{
    auto result = lua.script("return CheckField(10, Is.IntegerIn, 0, 100)").get<int>();
    EXPECT_EQ(result, 10);
}

TEST_F(CheckFieldTest, ValidIntegerAtBoundary)
{
    EXPECT_EQ(lua.script("return CheckField(0, Is.IntegerIn, 0, 100)").get<int>(), 0);
    EXPECT_EQ(lua.script("return CheckField(100, Is.IntegerIn, 0, 100)").get<int>(), 100);
}

TEST_F(CheckFieldTest, ValidUnsigned8)
{
    auto result = lua.script("return CheckField(255, Is.Unsigned8)").get<int>();
    EXPECT_EQ(result, 255);
}

TEST_F(CheckFieldTest, ValidFloatInRange)
{
    auto result = lua.script("return CheckField(5.5, Is.FloatIn, 0, 10)").get<double>();
    EXPECT_DOUBLE_EQ(result, 5.5);
}
