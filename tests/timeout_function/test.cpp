#include "lua_test_fixture.h"

#include <gtest/gtest.h>

#include <vector>

class TimeoutFunctionTest : public LuaTestFixture
{
protected:
    int              sleepCallCount = 0;
    std::vector<int> sleepArgs;

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
        lua.script("TimeoutFunction = require('lua/core/utils/timeout_function')");
    }
};

// =============================================================================
// Condition met immediately (routine returns false)
// =============================================================================

TEST_F(TimeoutFunctionTest, ConditionMetImmediately)
{
    lua.script(R"(
        TimeoutFunction(function() return false end, 100)
    )");
    EXPECT_EQ(sleepCallCount, 0);
}

// =============================================================================
// Condition met after N iterations
// =============================================================================

TEST_F(TimeoutFunctionTest, ConditionMetAfterThreeIterations)
{
    lua.script(R"(
        local count = 0
        TimeoutFunction(function()
            count = count + 1
            return count < 3
        end, 100)
    )");
    // Routine returns true 2 times (count=1,2), then false (count=3)
    // SleepFor called after each true return
    EXPECT_EQ(sleepCallCount, 2);
}

TEST_F(TimeoutFunctionTest, DefaultSleepMs)
{
    lua.script(R"(
        local count = 0
        TimeoutFunction(function()
            count = count + 1
            return count < 2
        end, 100)
    )");
    // Default sleep_ms is 10
    EXPECT_EQ(sleepCallCount, 1);
    EXPECT_EQ(sleepArgs[0], 10);
}

// =============================================================================
// Custom sleep_ms
// =============================================================================

TEST_F(TimeoutFunctionTest, CustomSleepMs)
{
    lua.script(R"(
        local count = 0
        TimeoutFunction(function()
            count = count + 1
            return count < 2
        end, 100, 25)
    )");
    EXPECT_EQ(sleepCallCount, 1);
    EXPECT_EQ(sleepArgs[0], 25);
}

// =============================================================================
// Timeout — routine never returns false
// =============================================================================

TEST_F(TimeoutFunctionTest, TimeoutThrowsInExecutionStage)
{
    // Context.info.stage is already Stage.execution (set by fixture)
    sol::protected_function_result result = lua.safe_script(R"(
        TimeoutFunction(function() return true end, 50, 10)
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
    std::string error = result.get<sol::error>().what();
    EXPECT_NE(error.find("Timeout"), std::string::npos);
    // With duration=50, sleep=10: loop runs 5 times before deadline <= 0
    EXPECT_EQ(sleepCallCount, 5);
}

TEST_F(TimeoutFunctionTest, TimeoutReturnsEarlyInNonExecutionStage)
{
    // Set stage to generation (not execution)
    lua.script("Context.info.stage = Stage.generation");

    // Should NOT throw, just return
    sol::protected_function_result result = lua.safe_script(R"(
        TimeoutFunction(function() return true end, 50, 10)
    )", sol::script_pass_on_error);
    EXPECT_TRUE(result.valid());
    // Still calls SleepFor until deadline runs out, then returns instead of erroring
    EXPECT_EQ(sleepCallCount, 5);
}

// =============================================================================
// Deadline calculation
// =============================================================================

TEST_F(TimeoutFunctionTest, DeadlineExactlyReached)
{
    // duration=30, sleep=10 -> 3 iterations before deadline hits 0
    sol::protected_function_result result = lua.safe_script(R"(
        TimeoutFunction(function() return true end, 30, 10)
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
    EXPECT_EQ(sleepCallCount, 3);
}

TEST_F(TimeoutFunctionTest, DeadlineWithUnevenDivision)
{
    // duration=25, sleep=10 -> after 1st: 15 remaining, after 2nd: 5 remaining, after 3rd: -5 (timeout)
    sol::protected_function_result result = lua.safe_script(R"(
        TimeoutFunction(function() return true end, 25, 10)
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
    EXPECT_EQ(sleepCallCount, 3);
}

// =============================================================================
// Invalid arguments
// =============================================================================

TEST_F(TimeoutFunctionTest, NonFunctionRoutineThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "TimeoutFunction('not a function', 100)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(TimeoutFunctionTest, NonNumberDurationThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "TimeoutFunction(function() return false end, 'bad')", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(TimeoutFunctionTest, NegativeDurationThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "TimeoutFunction(function() return false end, -10)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(TimeoutFunctionTest, NegativeSleepMsThrows)
{
    sol::protected_function_result result = lua.safe_script(
        "TimeoutFunction(function() return false end, 100, -5)", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}
