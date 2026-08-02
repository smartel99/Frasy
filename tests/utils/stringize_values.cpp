#include "lua_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// stringize_values module
// =============================================================================

class StringizeValuesTest : public LuaTestFixture
{
protected:
    void SetUp() override
    {
        LuaTestFixture::SetUp();
        lua.script("StringizeValues = require('lua/core/utils/stringize_values')");
    }
};

TEST_F(StringizeValuesTest, PackSingleByte)
{
    auto result = lua.script("return StringizeValues(65)").get<std::string>();
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 'A'); // 65 == 'A'
}

TEST_F(StringizeValuesTest, PackMultipleBytes)
{
    auto result = lua.script("return StringizeValues(72, 101, 108, 108, 111)").get<std::string>();
    EXPECT_EQ(result, "Hello");
}

TEST_F(StringizeValuesTest, PackZeroBytes)
{
    auto result = lua.script("return StringizeValues(0, 0, 0)").get<std::string>();
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], '\0');
    EXPECT_EQ(result[1], '\0');
    EXPECT_EQ(result[2], '\0');
}

TEST_F(StringizeValuesTest, PackEmptyVarargs)
{
    auto result = lua.script("return StringizeValues()").get<std::string>();
    EXPECT_EQ(result.size(), 0);
}
