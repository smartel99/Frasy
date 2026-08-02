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
