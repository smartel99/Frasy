#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class ExpectationMethodsTest : public OrchestratorTestFixture
{
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        createTest("Seq", "T1", true);
        setStage("execution");
        lua.script("Expectation = require('lua/core/framework/expectation/execution')");
        lua.script_file(getLuaBaseDir() + "/lua/core/framework/expectation/common.lua");
    }

    int expectationCount()
    {
        return lua.script(
            "return #Context.orchestrator.sequences['Seq'].tests['T1'].expectations").get<int>();
    }
};

TEST_F(ExpectationMethodsTest, ToBeTrue_StoresResult)
{
    lua.script("Expectation:New(true, 'flag'):ToBeTrue()");
    EXPECT_EQ(expectationCount(), 1);
    EXPECT_TRUE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].pass").get<bool>());
    EXPECT_EQ(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].method").get<std::string>(),
        "ToBeTrue");
}

TEST_F(ExpectationMethodsTest, ToBeEqual_StoresResult)
{
    lua.script("Expectation:New(42, 'answer'):ToBeEqual(42)");
    EXPECT_EQ(expectationCount(), 1);
    EXPECT_TRUE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].pass").get<bool>());
    EXPECT_EQ(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].method").get<std::string>(),
        "ToBeEqual");
}

TEST_F(ExpectationMethodsTest, ToBeEqual_FailStoresResult)
{
    lua.script("Expectation:New(41, 'answer'):ToBeEqual(42)");
    EXPECT_FALSE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].pass").get<bool>());
}

TEST_F(ExpectationMethodsTest, ToBeInRange_StoresMinMax)
{
    lua.script("Expectation:New(5, 'val'):ToBeInRange(0, 10)");
    EXPECT_EQ(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].method").get<std::string>(),
        "ToBeInRange");
    EXPECT_DOUBLE_EQ(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].min").get<double>(), 0);
    EXPECT_DOUBLE_EQ(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].max").get<double>(), 10);
}

TEST_F(ExpectationMethodsTest, MultipleExpectations_AllStored)
{
    lua.script(R"(
        Expectation:New(true, 'a'):ToBeTrue()
        Expectation:New(5, 'b'):ToBeEqual(5)
        Expectation:New(3.3, 'c'):ToBeInRange(3.0, 3.6)
    )");
    EXPECT_EQ(expectationCount(), 3);
    EXPECT_EQ(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].method").get<std::string>(),
        "ToBeTrue");
    EXPECT_EQ(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[2].method").get<std::string>(),
        "ToBeEqual");
    EXPECT_EQ(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[3].method").get<std::string>(),
        "ToBeInRange");
}

TEST_F(ExpectationMethodsTest, ToBeNear_StoresFields)
{
    lua.script("Expectation:New(10.2, 'v'):ToBeNear(10.0, 0.5)");
    auto method = lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].method").get<std::string>();
    EXPECT_EQ(method, "ToBeNear");
    EXPECT_TRUE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].pass").get<bool>());
}

TEST_F(ExpectationMethodsTest, ToBeInPercentage_StoresFields)
{
    lua.script("Expectation:New(3.3, 'vcc'):ToBeInPercentage(3.3, 5.0)");
    auto method = lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].method").get<std::string>();
    EXPECT_EQ(method, "ToBeInPercentage");
    EXPECT_DOUBLE_EQ(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].percentage").get<double>(), 5.0);
    // deviation = |3.3 * 5.0 / 100| = 0.165
    EXPECT_NEAR(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].min").get<double>(), 3.135, 1e-10);
    EXPECT_NEAR(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].max").get<double>(), 3.465, 1e-10);
}

TEST_F(ExpectationMethodsTest, ToMatch_StoresFields)
{
    lua.script("Expectation:New('hello123', 'serial'):ToMatch('%d+')");
    EXPECT_TRUE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].pass").get<bool>());
}
