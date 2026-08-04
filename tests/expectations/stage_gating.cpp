#include "lua_test_fixture.h"

#include <gtest/gtest.h>

class ExpectationsTest : public LuaTestFixture
{
protected:
    void SetUp() override
    {
        LuaTestFixture::SetUp();
        // Context.info.stage is already Stage.execution from fixture
        // Load the common expectation functions (they define globals)
        lua.script_file(getLuaBaseDir() + "/lua/core/framework/expectation/utils.lua");
    }
};

// =============================================================================
// Stage gating — non-execution stage returns {pass = true}
// =============================================================================

TEST_F(ExpectationsTest, NonExecutionStage_ShortCircuits)
{
    lua.script("Context.info.stage = Stage.generation");
    EXPECT_TRUE(lua.script("return ExpectToBeTrue(false).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeFalse(true).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeEqual(1, 2).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeNear(100, 0, 1).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeInRange(100, 0, 10).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeInPercentage(100, 0, 1).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeGreater(0, 10).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeGreaterOrEqual(0, 10).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeLesser(10, 0).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeLesserOrEqual(10, 0).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeType(42, 'string').pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeMatch(42, '%d+').pass").get<bool>());
}
