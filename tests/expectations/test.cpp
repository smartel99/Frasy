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
        lua.script_file(getLuaBaseDir() + "/lua/core/framework/expectation/common.lua");
    }
};

// =============================================================================
// ExpectToBeTrue
// =============================================================================

TEST_F(ExpectationsTest, ToBeTrue_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeTrue(true).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeTrue_FailsWithFalse)
{
    EXPECT_FALSE(lua.script("return ExpectToBeTrue(false).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeTrue_FailsWithNonBoolean)
{
    EXPECT_FALSE(lua.script("return ExpectToBeTrue(1).pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeTrue('true').pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeTrue(nil).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeTrue_Method)
{
    auto method = lua.script("return ExpectToBeTrue(true).method").get<std::string>();
    EXPECT_EQ(method, "ToBeTrue");
}

// =============================================================================
// ExpectToBeFalse
// =============================================================================

TEST_F(ExpectationsTest, ToBeFalse_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeFalse(false).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeFalse_FailsWithTrue)
{
    EXPECT_FALSE(lua.script("return ExpectToBeFalse(true).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeFalse_FailsWithNonBoolean)
{
    EXPECT_FALSE(lua.script("return ExpectToBeFalse(0).pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeFalse(nil).pass").get<bool>());
}

// =============================================================================
// ExpectToBeEqual
// =============================================================================

TEST_F(ExpectationsTest, ToBeEqual_SameValue)
{
    EXPECT_TRUE(lua.script("return ExpectToBeEqual(42, 42).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeEqual('hello', 'hello').pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeEqual(true, true).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeEqual_DifferentValue)
{
    EXPECT_FALSE(lua.script("return ExpectToBeEqual(42, 43).pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeEqual('hello', 'world').pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeEqual_DifferentType)
{
    EXPECT_FALSE(lua.script("return ExpectToBeEqual(42, '42').pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeEqual(1, true).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeEqual_Method)
{
    auto method = lua.script("return ExpectToBeEqual(1, 1).method").get<std::string>();
    EXPECT_EQ(method, "ToBeEqual");
}

// =============================================================================
// ExpectToBeNear
// =============================================================================

TEST_F(ExpectationsTest, ToBeNear_WithinDeviation)
{
    EXPECT_TRUE(lua.script("return ExpectToBeNear(10.5, 10.0, 1.0).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeNear(9.5, 10.0, 1.0).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeNear_AtBoundary)
{
    EXPECT_TRUE(lua.script("return ExpectToBeNear(11.0, 10.0, 1.0).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeNear(9.0, 10.0, 1.0).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeNear_OutsideDeviation)
{
    EXPECT_FALSE(lua.script("return ExpectToBeNear(11.1, 10.0, 1.0).pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeNear(8.9, 10.0, 1.0).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeNear_NonNumber)
{
    EXPECT_FALSE(lua.script("return ExpectToBeNear('10', 10.0, 1.0).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeNear_Fields)
{
    lua.script("r = ExpectToBeNear(5.0, 10.0, 2.0)");
    EXPECT_EQ(lua.script("return r.method").get<std::string>(), "ToBeNear");
    EXPECT_DOUBLE_EQ(lua.script("return r.min").get<double>(), 8.0);
    EXPECT_DOUBLE_EQ(lua.script("return r.max").get<double>(), 12.0);
}

// =============================================================================
// ExpectToBeInRange
// =============================================================================

TEST_F(ExpectationsTest, ToBeInRange_Within)
{
    EXPECT_TRUE(lua.script("return ExpectToBeInRange(5, 0, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeInRange_AtBoundary)
{
    EXPECT_TRUE(lua.script("return ExpectToBeInRange(0, 0, 10).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeInRange(10, 0, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeInRange_Outside)
{
    EXPECT_FALSE(lua.script("return ExpectToBeInRange(-1, 0, 10).pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeInRange(11, 0, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeInRange_NonNumber)
{
    EXPECT_FALSE(lua.script("return ExpectToBeInRange('5', 0, 10).pass").get<bool>());
}

// =============================================================================
// ExpectToBeInPercentage
// =============================================================================

TEST_F(ExpectationsTest, ToBeInPercentage_Within)
{
    // 3.3 ± 5% -> [3.135, 3.465]
    EXPECT_TRUE(lua.script("return ExpectToBeInPercentage(3.3, 3.3, 5.0).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeInPercentage(3.4, 3.3, 5.0).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeInPercentage_Outside)
{
    EXPECT_FALSE(lua.script("return ExpectToBeInPercentage(3.5, 3.3, 5.0).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeInPercentage_Fields)
{
    lua.script("r = ExpectToBeInPercentage(3.3, 3.3, 5.0)");
    EXPECT_EQ(lua.script("return r.method").get<std::string>(), "ToBeInPercentage");
    EXPECT_DOUBLE_EQ(lua.script("return r.percentage").get<double>(), 5.0);
    // deviation = |3.3 * 5 / 100| = 0.165
    EXPECT_NEAR(lua.script("return r.deviation").get<double>(), 0.165, 1e-10);
    EXPECT_NEAR(lua.script("return r.min").get<double>(), 3.135, 1e-10);
    EXPECT_NEAR(lua.script("return r.max").get<double>(), 3.465, 1e-10);
}

// =============================================================================
// ExpectToBeGreater / ExpectToBeGreaterOrEqual
// =============================================================================

TEST_F(ExpectationsTest, ToBeGreater_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeGreater(11, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeGreater_FailsAtBoundary)
{
    EXPECT_FALSE(lua.script("return ExpectToBeGreater(10, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeGreater_FailsBelow)
{
    EXPECT_FALSE(lua.script("return ExpectToBeGreater(9, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeGreaterOrEqual_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeGreaterOrEqual(10, 10).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeGreaterOrEqual(11, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeGreaterOrEqual_Fails)
{
    EXPECT_FALSE(lua.script("return ExpectToBeGreaterOrEqual(9, 10).pass").get<bool>());
}

// =============================================================================
// ExpectToBeLesser / ExpectToBeLesserOrEqual
// =============================================================================

TEST_F(ExpectationsTest, ToBeLesser_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeLesser(9, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeLesser_FailsAtBoundary)
{
    EXPECT_FALSE(lua.script("return ExpectToBeLesser(10, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeLesser_FailsAbove)
{
    EXPECT_FALSE(lua.script("return ExpectToBeLesser(11, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeLesserOrEqual_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeLesserOrEqual(10, 10).pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeLesserOrEqual(9, 10).pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeLesserOrEqual_Fails)
{
    EXPECT_FALSE(lua.script("return ExpectToBeLesserOrEqual(11, 10).pass").get<bool>());
}

// =============================================================================
// ExpectToBeType
// =============================================================================

TEST_F(ExpectationsTest, ToBeType_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeType(42, 'number').pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeType('hi', 'string').pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeType({}, 'table').pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeType(true, 'boolean').pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeType_Fails)
{
    EXPECT_FALSE(lua.script("return ExpectToBeType(42, 'string').pass").get<bool>());
    EXPECT_FALSE(lua.script("return ExpectToBeType('hi', 'number').pass").get<bool>());
}

TEST_F(ExpectationsTest, ToBeType_Fields)
{
    lua.script("r = ExpectToBeType(42, 'string')");
    EXPECT_EQ(lua.script("return r.method").get<std::string>(), "ToBeType");
    EXPECT_EQ(lua.script("return r.expected").get<std::string>(), "string");
    EXPECT_EQ(lua.script("return r.type").get<std::string>(), "number");
}

// =============================================================================
// ExpectToBeMatch
// =============================================================================

TEST_F(ExpectationsTest, ToMatch_Passes)
{
    EXPECT_TRUE(lua.script("return ExpectToBeMatch('hello world', 'hello').pass").get<bool>());
    EXPECT_TRUE(lua.script("return ExpectToBeMatch('test123', '%d+').pass").get<bool>());
}

TEST_F(ExpectationsTest, ToMatch_Fails)
{
    EXPECT_FALSE(lua.script("return ExpectToBeMatch('hello', 'xyz').pass").get<bool>());
}

TEST_F(ExpectationsTest, ToMatch_NonStringFails)
{
    EXPECT_FALSE(lua.script("return ExpectToBeMatch(123, '%d+').pass").get<bool>());
}

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
