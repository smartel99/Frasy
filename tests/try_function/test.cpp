#include "lua_test_fixture.h"

#include <gtest/gtest.h>

#include <vector>

class TryFunctionTest : public LuaTestFixture
{
protected:
    int                sleepCallCount = 0;
    std::vector<int>   sleepArgs;

    void SetUp() override
    {
        LuaTestFixture::SetUp();

        // Override SleepFor with a tracking mock
        lua.set_function("SleepFor", [this](int ms) {
            sleepCallCount++;
            sleepArgs.push_back(ms);
        });

        // Load dependencies
        lua.script("Is = require('lua/core/utils/is')");
        lua.script("CheckField = require('lua/core/utils/check_field')");
        lua.script("TryFunction = require('lua/core/utils/try_function')");
    }
};

// =============================================================================
// Success on first try
// =============================================================================

TEST_F(TryFunctionTest, SucceedsOnFirstTry)
{
    auto result = lua.script(R"(
        return TryFunction(function(try) return true end)
    )").get<bool>();
    EXPECT_TRUE(result);
    EXPECT_EQ(sleepCallCount, 0); // No sleep before first attempt
}

TEST_F(TryFunctionTest, SucceedsOnFirstTry_ReturnsExtraValue)
{
    auto result = lua.script(R"(
        local ok, val = TryFunction(function(try) return true, 42 end)
        return val
    )").get<int>();
    EXPECT_EQ(result, 42);
}

// =============================================================================
// Success on Nth try
// =============================================================================

TEST_F(TryFunctionTest, SucceedsOnThirdTry)
{
    auto result = lua.script(R"(
        local attempts = 0
        local ok = TryFunction(function(try)
            attempts = attempts + 1
            return attempts == 3
        end)
        return ok
    )").get<bool>();
    EXPECT_TRUE(result);
    // SleepFor called before 2nd and 3rd attempts (not before 1st)
    EXPECT_EQ(sleepCallCount, 2);
}

TEST_F(TryFunctionTest, SucceedsOnSecondTry_DefaultDelay)
{
    lua.script(R"(
        TryFunction(function(try) return try == 2 end)
    )");
    // Default delay is 10ms, called once (before 2nd attempt)
    EXPECT_EQ(sleepCallCount, 1);
    EXPECT_EQ(sleepArgs[0], 10);
}

// =============================================================================
// Failure — exhausts retries
// =============================================================================

TEST_F(TryFunctionTest, FailsAfterDefaultMaxTries)
{
    auto result = lua.script(R"(
        return TryFunction(function(try) return false end)
    )").get<bool>();
    EXPECT_FALSE(result);
    // Default maxTryCount=3, SleepFor called before 2nd and 3rd attempt
    EXPECT_EQ(sleepCallCount, 2);
}

TEST_F(TryFunctionTest, FailsWithRaiseError)
{
    sol::protected_function_result result = lua.safe_script(R"(
        TryFunction(function(try) return false end, { raiseError = true })
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
    std::string error = result.get<sol::error>().what();
    EXPECT_NE(error.find("tries limit"), std::string::npos);
}

// =============================================================================
// Custom options
// =============================================================================

TEST_F(TryFunctionTest, CustomMaxTryCount)
{
    lua.script(R"(
        TryFunction(function(try) return false end, { maxTryCount = 5 })
    )");
    // 5 attempts, SleepFor before 2nd, 3rd, 4th, 5th = 4 calls
    EXPECT_EQ(sleepCallCount, 4);
}

TEST_F(TryFunctionTest, CustomDelay)
{
    lua.script(R"(
        TryFunction(function(try) return false end, { delay = 50 })
    )");
    // Default 3 tries, delay=50 between each
    EXPECT_EQ(sleepCallCount, 2);
    EXPECT_EQ(sleepArgs[0], 50);
    EXPECT_EQ(sleepArgs[1], 50);
}

TEST_F(TryFunctionTest, ZeroDelay_NoSleep)
{
    lua.script(R"(
        TryFunction(function(try) return false end, { delay = 0 })
    )");
    // delay=0 means SleepFor is not called
    EXPECT_EQ(sleepCallCount, 0);
}

TEST_F(TryFunctionTest, SingleTry)
{
    auto result = lua.script(R"(
        return TryFunction(function(try) return false end, { maxTryCount = 1 })
    )").get<bool>();
    EXPECT_FALSE(result);
    EXPECT_EQ(sleepCallCount, 0); // Only 1 attempt, no retries
}

// =============================================================================
// Try counter passed to function
// =============================================================================

TEST_F(TryFunctionTest, TryCounterPassedToFunction)
{
    auto result = lua.script(R"(
        local tries = {}
        TryFunction(function(try)
            table.insert(tries, try)
            return false
        end, { maxTryCount = 4 })
        return tries[1], tries[2], tries[3], tries[4]
    )");
    EXPECT_EQ(result.get<int>(0), 1);
    EXPECT_EQ(result.get<int>(1), 2);
    EXPECT_EQ(result.get<int>(2), 3);
    EXPECT_EQ(result.get<int>(3), 4);
}

// =============================================================================
// Invalid arguments
// =============================================================================

TEST_F(TryFunctionTest, NonFunctionThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "TryFunction('not a function')", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(TryFunctionTest, NonTableOptThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "TryFunction(function() return true end, 'bad')", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(TryFunctionTest, InvalidMaxTryCountThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "TryFunction(function() return true end, { maxTryCount = -1 })", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(TryFunctionTest, InvalidDelayThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "TryFunction(function() return true end, { delay = -5 })", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}
