#include "lua_test_fixture.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>

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

// =============================================================================
// Error message contains useful information
// =============================================================================

TEST_F(CheckFieldTest, ErrorMessageContainsValueAndLocation)
{
    // Call CheckField from a named Lua function so debug info resolves
    lua.script(R"(
        function doCheck()
            local myVar = "bad_value"
            CheckField(myVar, Is.Integer)
        end
    )");
    sol::protected_function_result result = lua.safe_script(
        "doCheck()", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
    std::string error = result.get<sol::error>().what();
    // Error should contain "CheckField" indicating which function failed
    EXPECT_NE(error.find("CheckField"), std::string::npos);
    // Error should contain the value that failed
    EXPECT_NE(error.find("bad_value"), std::string::npos);
}

TEST_F(CheckFieldTest, ErrorMessageFromFileContainsLocationAndValue)
{
    // Write a Lua file with a CheckField call so source info is available
    // check_field uses debug.getinfo to get source file and line
    std::filesystem::path luaFile = std::filesystem::path(getLuaBaseDir()) / "test_check_field_err.lua";
    {
        std::ofstream f(luaFile);
        f << "local function myFunc()\n";
        f << "    local voltage = -5\n";
        f << "    CheckField(voltage, Is.Unsigned)\n";
        f << "end\n";
        f << "return myFunc\n";
    }

    // Convert path to forward slashes for Lua
    std::string luaPath = luaFile.string();
    std::replace(luaPath.begin(), luaPath.end(), '\\', '/');

    sol::protected_function_result result = lua.safe_script(
        "local fn = dofile('" + luaPath + "'); fn()", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
    std::string error = result.get<sol::error>().what();
    // Should contain "CheckField" (the function name)
    EXPECT_NE(error.find("CheckField"), std::string::npos);
    // Should contain the failed value
    EXPECT_NE(error.find("-5"), std::string::npos);
    // Should contain the source file reference
    EXPECT_NE(error.find("test_check_field_err.lua"), std::string::npos);

    // Clean up temp file
    std::filesystem::remove(luaFile);
}
