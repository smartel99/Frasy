#include "lua_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// bitwise module
// =============================================================================

class BitwiseTest : public LuaTestFixture
{
protected:
    void SetUp() override
    {
        LuaTestFixture::SetUp();
        lua.script("Bitwise = require('lua/core/utils/bitwise')");
    }
};

TEST_F(BitwiseTest, Extract)
{
    // Extract bit 0 from 0b0101 (5) -> 1
    EXPECT_EQ(lua.script("return Bitwise.Extract(0, 5)").get<int>(), 1);
    // Extract bit 1 from 0b0101 (5) -> 0
    EXPECT_EQ(lua.script("return Bitwise.Extract(1, 5)").get<int>(), 0);
    // Extract bit 2 from 0b0101 (5) -> 1
    EXPECT_EQ(lua.script("return Bitwise.Extract(2, 5)").get<int>(), 1);
    // Extract bit 3 from 0b0101 (5) -> 0
    EXPECT_EQ(lua.script("return Bitwise.Extract(3, 5)").get<int>(), 0);
}

TEST_F(BitwiseTest, Inject)
{
    // Inject 1 at bit 0 into 0 -> 1
    EXPECT_EQ(lua.script("return Bitwise.Inject(0, 1, 0)").get<int>(), 1);
    // Inject 1 at bit 2 into 0 -> 4
    EXPECT_EQ(lua.script("return Bitwise.Inject(2, 1, 0)").get<int>(), 4);
    // Inject 0 at bit 0 into 0b0101 (5) -> 0b0100 (4)
    EXPECT_EQ(lua.script("return Bitwise.Inject(0, 0, 5)").get<int>(), 4);
    // Inject 1 at bit 1 into 0b0101 (5) -> 0b0111 (7)
    EXPECT_EQ(lua.script("return Bitwise.Inject(1, 1, 5)").get<int>(), 7);
    // Inject 1 at bit 2 into 0b0101 (5) -> still 0b0101 (5), already set
    EXPECT_EQ(lua.script("return Bitwise.Inject(2, 1, 5)").get<int>(), 5);
}

TEST_F(BitwiseTest, InjectThenExtract)
{
    // Set bit 3, then extract it
    auto result = lua.script(R"(
        local val = Bitwise.Inject(3, 1, 0)
        return Bitwise.Extract(3, val)
    )").get<int>();
    EXPECT_EQ(result, 1);
}
