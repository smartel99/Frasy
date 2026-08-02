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
// Invalid fields — should throw an error
// =============================================================================

TEST_F(CheckFieldTest, InvalidIntegerThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "CheckField('hello', Is.Integer)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(CheckFieldTest, InvalidStringThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "CheckField(123, Is.String)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(CheckFieldTest, InvalidBooleanThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "CheckField(1, Is.Boolean)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(CheckFieldTest, InvalidRangeThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "CheckField(101, Is.IntegerIn, 0, 100)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(CheckFieldTest, InvalidNegativeForUnsignedThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "CheckField(-1, Is.Unsigned)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(CheckFieldTest, InvalidUnsigned8OverflowThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "CheckField(256, Is.Unsigned8)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(CheckFieldTest, InvalidNilForIntegerThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "CheckField(nil, Is.Integer)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}
