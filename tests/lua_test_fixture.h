/**
 * @file    lua_test_fixture.h
 * @brief   Shared Google Test fixture for Lua script testing.
 *
 * Provides a sol::state with common mocks and framework globals pre-loaded,
 * ready for loading and testing individual Lua modules.
 */

#ifndef FRASY_TESTS_LUA_TEST_FIXTURE_H
#define FRASY_TESTS_LUA_TEST_FIXTURE_H

#include <gtest/gtest.h>
#include <sol/sol.hpp>

#include <filesystem>
#include <string>
#include <vector>

/**
 * Base test fixture that sets up a Lua state with:
 * - Standard libraries (base, string, table, math, io, debug, package)
 * - C++ mock functions for bindings that don't exist in test context
 * - Framework globals (Stage, Context, Orchestrator, exception constructors)
 * - Utility globals from global.lua (Print, ToString, Equals, Traverse, LineSplit, ToInt)
 * - package.path configured to resolve require("lua/core/...") from the test binary dir
 */
class LuaTestFixture : public ::testing::Test
{
protected:
    sol::state lua;

    void SetUp() override
    {
        // Open standard libraries
        lua.open_libraries(
          sol::lib::base,
          sol::lib::string,
          sol::lib::table,
          sol::lib::math,
          sol::lib::io,
          sol::lib::debug,
          sol::lib::package);

        // Configure package.path so require("lua/core/...") resolves correctly.
        // The working directory for tests is set to the tests/ binary dir
        // where lua/core/ is synced by the sync_test_lua target.
        std::string luaDir = getLuaBaseDir();
        std::string path   = luaDir + "/?.lua;" + luaDir + "/?/init.lua";
        lua["package"]["path"] = path;

        // Register C++ mock functions
        registerMocks();

        // Load framework globals
        lua.script_file(luaDir + "/lua/core/framework/stage.lua");
        lua.script_file(luaDir + "/lua/core/framework/exception.lua");
        lua.script_file(luaDir + "/lua/core/utils/global.lua");

        // Set up minimal Context
        lua.script(R"(
            Context = {
                info = {
                    serial = "",
                    uut = 0,
                    stage = Stage.execution,
                    time = { start = 0, stop = 0, elapsed = 0, process = 0 },
                    orchestrator = { version = "1.2.0", date = "5/15/2024" },
                    user = { version = "0.0.0", date = "12/31/1969" }
                },
                values = {}
            }
        )");

        // Set up minimal Orchestrator with no-op functions
        lua.script(R"(
            Orchestrator = {
                AddExpectationResult = function(result) end,
                GetScope = function() return "" end,
                SetValue = function(scope, name, value) end,
                GetSequenceScopeRequirement = function(name) return nil end,
                GetTestScopeRequirement = function(name) return nil end,
                GetSyncRequirement = function() return {} end,
                AddSyncRequirement = function(req) end,
                CreateSequence = function(name, func, source, line) end,
                CreateTest = function(name, func, source, line) end,
            }
        )");
    }

    /**
     * Get the base directory where Lua files are synced.
     * This is the working directory set by gtest_discover_tests,
     * which should be the tests/ binary directory.
     */
    static std::string getLuaBaseDir()
    {
        // The working directory is set to the tests binary dir parent
        // (where lua/core/ lives). We use the current path.
        return std::filesystem::current_path().string();
    }

    /**
     * Register mock C++ functions that Lua scripts expect to exist.
     * Override in derived fixtures if you need custom mock behavior.
     */
    virtual void registerMocks()
    {
        lua.set_function("SleepFor", [](int /*ms*/) {});
        lua.set_function("Hash", [](const std::string& /*str*/) -> int { return 0; });
        lua.set_function("DirList", [this](const std::string& /*path*/) { return lua.create_table(); });
        lua.set_function("SaveAsJson", [](sol::table /*table*/, const std::string& /*filepath*/) {});
        lua.set_function("CombineAndBitcast", [](sol::table /*data*/) -> double { return 0.0; });
        lua.set_function("ShowExpectation", [](sol::table /*result*/) {});
        lua.set_function("__exclusive", [](int /*value*/, sol::function func) { func(); });
        lua.set_function("__once", [](int /*hash*/, sol::function func) { func(); });
    }
};

#endif // FRASY_TESTS_LUA_TEST_FIXTURE_H
