#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class ExpectationExportAsTest : public OrchestratorTestFixture
{
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        createTest("Seq", "T1", true);
        setStage("execution");
        lua.script("Expectation = require('lua/core/framework/expectation/expectation')");
        lua.script_file(getLuaBaseDir() + "/lua/core/framework/expectation/utils.lua");
    }
};

TEST_F(ExpectationExportAsTest, ExportAs_StoresValueInOrchestrator)
{
    lua.script(R"(
        Expectation:New(3.3, "voltage"):ToBeInRange(3.0, 3.6):ExportAs("measured_voltage")
    )");
    auto val = lua.script(R"(
        local scope = Orchestrator.GetScope()
        return Orchestrator.GetValue(scope, "measured_voltage")
    )").get<double>();
    EXPECT_DOUBLE_EQ(val, 3.3);
}

TEST_F(ExpectationExportAsTest, ExportAs_WorksEvenWhenFailing)
{
    // The value is exported regardless of pass/fail
    lua.script(R"(
        Expectation:New(100, "val"):ToBeInRange(0, 50):ExportAs("out_of_range_val")
    )");
    auto val = lua.script(R"(
        local scope = Orchestrator.GetScope()
        return Orchestrator.GetValue(scope, "out_of_range_val")
    )").get<int>();
    EXPECT_EQ(val, 100);
}

TEST_F(ExpectationExportAsTest, ExportAs_StringValue)
{
    lua.script(R"(
        Expectation:New("SN12345", "serial"):ToMatch("SN%d+"):ExportAs("serial_number")
    )");
    auto val = lua.script(R"(
        local scope = Orchestrator.GetScope()
        return Orchestrator.GetValue(scope, "serial_number")
    )").get<std::string>();
    EXPECT_EQ(val, "SN12345");
}
