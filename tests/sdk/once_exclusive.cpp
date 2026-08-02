#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class SdkOnceExclusiveTest : public OrchestratorTestFixture
{
protected:
    int onceCallCount = 0;
    int exclusiveCallCount = 0;
    int hashCallCount = 0;

    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        createTest("Seq", "T1", true);
        setStage("execution");

        // Override mocks with tracking
        lua.set_function("Hash", [this](const std::string&) -> int {
            hashCallCount++;
            return 12345;
        });
        lua.set_function("__once", [this](int /*hash*/, sol::function func) {
            onceCallCount++;
            func();
        });
        lua.set_function("__exclusive", [this](int /*value*/, sol::function func) {
            exclusiveCallCount++;
            func();
        });
    }
};

TEST_F(SdkOnceExclusiveTest, Once_CallsFunction)
{
    lua.script(R"(
        __called = false
        Once(function() __called = true end)
    )");
    EXPECT_TRUE(lua.script("return __called").get<bool>());
}

TEST_F(SdkOnceExclusiveTest, Once_UsesHashAndOnceMock)
{
    lua.script("Once(function() end)");
    EXPECT_EQ(hashCallCount, 1);
    EXPECT_EQ(onceCallCount, 1);
}

TEST_F(SdkOnceExclusiveTest, Exclusive_CallsFunction)
{
    lua.script(R"(
        __called = false
        Exclusive(1, function() __called = true end)
    )");
    EXPECT_TRUE(lua.script("return __called").get<bool>());
}

TEST_F(SdkOnceExclusiveTest, Exclusive_PassesMutexId)
{
    lua.set_function("__exclusive", [this](int value, sol::function func) {
        exclusiveCallCount = value; // store the mutex id for verification
        func();
    });
    lua.script("Exclusive(42, function() end)");
    EXPECT_EQ(exclusiveCallCount, 42);
}

TEST_F(SdkOnceExclusiveTest, Exclusive_ReturnsFunctionResult)
{
    lua.set_function("__exclusive", [](int, sol::function func) { return func(); });
    auto val = lua.script(R"(
        return Exclusive(1, function() return 99 end)
    )").get<int>();
    EXPECT_EQ(val, 99);
}
