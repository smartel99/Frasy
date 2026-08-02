#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class ExpectationChainingTest : public OrchestratorTestFixture
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
};

TEST_F(ExpectationChainingTest, NotThenMethod)
{
    // Not():ToBeEqual(x) — inverted check
    lua.script("Expectation:New(1, 'x'):Not():ToBeEqual(2)");
    // pass=false (1 != 2), inverted=true -> effectively passes
    EXPECT_FALSE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].pass").get<bool>());
    EXPECT_TRUE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].inverted").get<bool>());
}

TEST_F(ExpectationChainingTest, MethodThenExportAs)
{
    lua.script(R"(
        Expectation:New(3.3, "vcc"):ToBeInPercentage(3.3, 5.0):ExportAs("vcc_measured")
    )");
    auto val = lua.script(R"(
        local scope = Orchestrator.GetScope()
        return Orchestrator.GetValue(scope, "vcc_measured")
    )").get<double>();
    EXPECT_DOUBLE_EQ(val, 3.3);
}

TEST_F(ExpectationChainingTest, NotThenMethodThenMandatory_Throws)
{
    // NOTE: Currently Mandatory must be set BEFORE the assertion method to enforce.
    // TODO: Mandatory() should enforce retroactively when called after the assertion.
    //       See GitHub issue: "Expectation:Mandatory() should enforce retroactively"
    // 1 Not():Mandatory():ToBeEqual(1) -> pass=true, inverted=true -> pass == inverted -> throw
    sol::protected_function_result result = lua.safe_script(R"(
        Expectation:New(1, "x"):Not():Mandatory():ToBeEqual(1)
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid());
}

TEST_F(ExpectationChainingTest, FullChain_PassingWithExport)
{
    sol::protected_function_result result = lua.safe_script(R"(
        Expectation:New(5.0, "current")
            :Mandatory()
            :ToBeInRange(4.0, 6.0)
            :ExportAs("measured_current")
    )", sol::script_pass_on_error);
    EXPECT_TRUE(result.valid());
    auto val = lua.script(R"(
        local scope = Orchestrator.GetScope()
        return Orchestrator.GetValue(scope, "measured_current")
    )").get<double>();
    EXPECT_DOUBLE_EQ(val, 5.0);
}

TEST_F(ExpectationChainingTest, FullChain_FailingMandatoryWithOnErrorExtra)
{
    sol::protected_function_result result = lua.safe_script(R"(
        Expectation:New(100, "temp")
            :Mandatory()
            :OnErrorExtra({ unit = "celsius" })
            :ToBeInRange(20, 80)
    )", sol::script_pass_on_error);
    EXPECT_FALSE(result.valid()); // Mandatory + fail = throw

    // Verify the expectation was stored with extra data before the throw
    auto extra = lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].extra.unit"
    ).get<std::string>();
    EXPECT_EQ(extra, "celsius");

    // Verify the result fields are also present
    EXPECT_FALSE(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].pass").get<bool>());
}
