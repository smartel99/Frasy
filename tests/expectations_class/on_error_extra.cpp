#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class ExpectationOnErrorExtraTest : public OrchestratorTestFixture
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

TEST_F(ExpectationOnErrorExtraTest, OnErrorExtra_AppendsOnFailure)
{
    lua.script(R"(
        Expectation:New(100, "val"):ToBeInRange(0, 50):OnErrorExtra({ debug = "out of range" })
    )");
    auto extra = lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].extra.debug"
    ).get<std::string>();
    EXPECT_EQ(extra, "out of range");
}

TEST_F(ExpectationOnErrorExtraTest, OnErrorExtra_NotAppliedOnPass)
{
    lua.script(R"(
        Expectation:New(5, "val"):ToBeInRange(0, 10):OnErrorExtra({ debug = "should not appear" })
    )");
    auto isNil = lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].extra == nil"
    ).get<bool>();
    EXPECT_TRUE(isNil);
}

TEST_F(ExpectationOnErrorExtraTest, OnErrorExtra_MultipleKeys)
{
    lua.script(R"(
        Expectation:New(false, "flag"):ToBeTrue():OnErrorExtra({ a = 1, b = "two" })
    )");
    EXPECT_EQ(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].extra.a").get<int>(), 1);
    EXPECT_EQ(lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].extra.b").get<std::string>(), "two");
}

TEST_F(ExpectationOnErrorExtraTest, OnErrorExtra_BeforeMethod_AppliedAfterFail)
{
    // OnErrorExtra can be called before or after the assertion method
    // When called before, onErrorExtra is stored and applied after the method runs
    lua.script(R"(
        Expectation:New(false, "flag"):OnErrorExtra({ info = "pre-set" }):ToBeTrue()
    )");
    auto extra = lua.script(
        "return Context.orchestrator.sequences['Seq'].tests['T1'].expectations[1].extra.info"
    ).get<std::string>();
    EXPECT_EQ(extra, "pre-set");
}
