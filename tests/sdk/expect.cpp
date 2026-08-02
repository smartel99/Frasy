#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class SdkExpectTest : public OrchestratorTestFixture
{
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        createTest("Seq", "T1", true);
        setStage("execution");
        // Load execution expectation (needed for Expect() to work)
        lua.script("package.loaded['lua/core/framework/expectation/module'] = nil");
        lua.script_file(getLuaBaseDir() + "/lua/core/framework/expectation/common.lua");
        // Reload the SDK's Expect which uses the expectation module
        lua.script("Expectation = require('lua/core/framework/expectation/execution')");
        // Re-define Expect to use the execution Expectation
        lua.script(R"(
            function Expect(value, name, opt)
                return Expectation:New(value, name, opt)
            end
        )");
    }
};

TEST_F(SdkExpectTest, Expect_ReturnsExpectationObject)
{
    auto t = lua.script("return type(Expect(42, 'val'))").get<std::string>();
    EXPECT_EQ(t, "table");
}

TEST_F(SdkExpectTest, Expect_CanChainMethods)
{
    lua.script("Expect(3.3, 'voltage'):ToBeInRange(3.0, 3.6)");
    auto count = lua.script(
        "return #Context.orchestrator.sequences['Seq'].tests['T1'].expectations").get<int>();
    EXPECT_EQ(count, 1);
}

TEST_F(SdkExpectTest, Expect_WithOpt)
{
    lua.script(R"(
        Expect(1, "x", { note = "my note" }):ToBeEqual(1)
    )");
    auto note = lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].note").get<std::string>();
    EXPECT_EQ(note, "my note");
}

TEST_F(SdkExpectTest, Expect_MultipleInOneTest)
{
    lua.script(R"(
        Expect(true, "flag"):ToBeTrue()
        Expect(3.3, "vcc"):ToBeInPercentage(3.3, 5.0)
        Expect("OK", "status"):ToBeEqual("OK")
    )");
    auto count = lua.script(
        "return #Context.orchestrator.sequences['Seq'].tests['T1'].expectations").get<int>();
    EXPECT_EQ(count, 3);
}
