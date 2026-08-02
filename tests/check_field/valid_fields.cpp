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
// Valid fields — should return the value passed in
// =============================================================================

TEST_F(CheckFieldTest, ValidInteger)
{
    auto result = lua.script("return CheckField(5, Is.Integer)").get<int>();
    EXPECT_EQ(result, 5);
}

TEST_F(CheckFieldTest, ValidString)
{
    auto result = lua.script("return CheckField('hello', Is.String)").get<std::string>();
    EXPECT_EQ(result, "hello");
}

TEST_F(CheckFieldTest, ValidBoolean)
{
    auto result = lua.script("return CheckField(true, Is.Boolean)").get<bool>();
    EXPECT_TRUE(result);
}

TEST_F(CheckFieldTest, ValidTable)
{
    auto result = lua.script("return type(CheckField({1,2,3}, Is.Table))").get<std::string>();
    EXPECT_EQ(result, "table");
}

TEST_F(CheckFieldTest, ValidFloat)
{
    auto result = lua.script("return CheckField(3.14, Is.Float)").get<double>();
    EXPECT_DOUBLE_EQ(result, 3.14);
}

TEST_F(CheckFieldTest, ValidUnsigned)
{
    auto result = lua.script("return CheckField(0, Is.Unsigned)").get<int>();
    EXPECT_EQ(result, 0);
}
