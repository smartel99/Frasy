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
// Maybe integration — nil should pass, valid should pass, invalid should throw
// =============================================================================

TEST_F(CheckFieldTest, MaybeNilPasses)
{
    // nil is valid for Maybe — should not throw and return nil
    sol::protected_function_result result = lua.safe_script(
        "return CheckField(nil, Maybe, Is.Integer)", sol::script_pass_on_error);
    EXPECT_TRUE(result.valid());
}

TEST_F(CheckFieldTest, MaybeValidValuePasses)
{
    auto result = lua.script("return CheckField(42, Maybe, Is.Integer)").get<int>();
    EXPECT_EQ(result, 42);
}

TEST_F(CheckFieldTest, MaybeInvalidValueThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "CheckField('oops', Maybe, Is.Integer)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(CheckFieldTest, MaybeWithRangeNilPasses)
{
    sol::protected_function_result result = lua.safe_script(
        "return CheckField(nil, Maybe, Is.IntegerIn, 0, 100)", sol::script_pass_on_error);
    EXPECT_TRUE(result.valid());
}

TEST_F(CheckFieldTest, MaybeWithRangeValidPasses)
{
    auto result = lua.script("return CheckField(50, Maybe, Is.IntegerIn, 0, 100)").get<int>();
    EXPECT_EQ(result, 50);
}

TEST_F(CheckFieldTest, MaybeWithRangeInvalidThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "CheckField(200, Maybe, Is.IntegerIn, 0, 100)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}
