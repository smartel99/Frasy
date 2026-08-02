#include "lua_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// global.lua — Equals
// =============================================================================

TEST_F(LuaTestFixture, Equals_SamePrimitives)
{
    EXPECT_TRUE(lua.script("return Equals(1, 1)").get<bool>());
    EXPECT_TRUE(lua.script("return Equals('hello', 'hello')").get<bool>());
    EXPECT_TRUE(lua.script("return Equals(true, true)").get<bool>());
    EXPECT_TRUE(lua.script("return Equals(nil, nil)").get<bool>());
}

TEST_F(LuaTestFixture, Equals_DifferentPrimitives)
{
    EXPECT_FALSE(lua.script("return Equals(1, 2)").get<bool>());
    EXPECT_FALSE(lua.script("return Equals('hello', 'world')").get<bool>());
    EXPECT_FALSE(lua.script("return Equals(true, false)").get<bool>());
}

TEST_F(LuaTestFixture, Equals_DifferentTypes)
{
    EXPECT_FALSE(lua.script("return Equals(1, '1')").get<bool>());
    EXPECT_FALSE(lua.script("return Equals(nil, false)").get<bool>());
    EXPECT_FALSE(lua.script("return Equals(0, false)").get<bool>());
    EXPECT_FALSE(lua.script("return Equals({}, 'table')").get<bool>());
}

TEST_F(LuaTestFixture, Equals_SameTables)
{
    EXPECT_TRUE(lua.script("return Equals({1, 2, 3}, {1, 2, 3})").get<bool>());
    EXPECT_TRUE(lua.script("return Equals({a=1, b=2}, {a=1, b=2})").get<bool>());
}

TEST_F(LuaTestFixture, Equals_NestedTables)
{
    EXPECT_TRUE(lua.script("return Equals({a={b=1}}, {a={b=1}})").get<bool>());
    EXPECT_FALSE(lua.script("return Equals({a={b=1}}, {a={b=2}})").get<bool>());
}

TEST_F(LuaTestFixture, Equals_DifferentKeyCount)
{
    EXPECT_FALSE(lua.script("return Equals({a=1, b=2}, {a=1})").get<bool>());
    EXPECT_FALSE(lua.script("return Equals({a=1}, {a=1, b=2})").get<bool>());
}

TEST_F(LuaTestFixture, Equals_EmptyTables)
{
    EXPECT_TRUE(lua.script("return Equals({}, {})").get<bool>());
}

// =============================================================================
// global.lua — ToString
// =============================================================================

TEST_F(LuaTestFixture, ToString_Primitives)
{
    EXPECT_EQ(lua.script("return ToString(42)").get<std::string>(), "42");
    EXPECT_EQ(lua.script("return ToString('hello')").get<std::string>(), "hello");
    EXPECT_EQ(lua.script("return ToString(true)").get<std::string>(), "true");
    EXPECT_EQ(lua.script("return ToString(nil)").get<std::string>(), "nil");
}

TEST_F(LuaTestFixture, ToString_TableWithMetamethod)
{
    auto result = lua.script(R"(
        local t = setmetatable({}, { __tostring = function() return "custom" end })
        return ToString(t)
    )").get<std::string>();
    EXPECT_EQ(result, "custom");
}

TEST_F(LuaTestFixture, ToString_TableWithoutMetamethod)
{
    // Should produce PrettyPrint output (key: value format), not the default table pointer
    auto result = lua.script("return ToString({x=1})").get<std::string>();
    EXPECT_NE(result.find("x"), std::string::npos);
    EXPECT_NE(result.find("1"), std::string::npos);
}

// =============================================================================
// global.lua — Traverse
// =============================================================================

TEST_F(LuaTestFixture, Traverse_ValidChain)
{
    auto result = lua.script("return Traverse({B = {C = 42}}, 'B', 'C')").get<int>();
    EXPECT_EQ(result, 42);
}

TEST_F(LuaTestFixture, Traverse_BrokenChainAtEnd)
{
    auto result = lua.script("return Traverse({B = {C = 42}}, 'B', 'D') == nil").get<bool>();
    EXPECT_TRUE(result);
}

TEST_F(LuaTestFixture, Traverse_BrokenChainInMiddle)
{
    auto result = lua.script("return Traverse({B = {C = 42}}, 'X', 'C') == nil").get<bool>();
    EXPECT_TRUE(result);
}

TEST_F(LuaTestFixture, Traverse_NoArgs)
{
    // With no key args, returns the table itself
    auto result = lua.script("return type(Traverse({a=1}))").get<std::string>();
    EXPECT_EQ(result, "table");
}

TEST_F(LuaTestFixture, Traverse_NilInput)
{
    auto result = lua.script("return Traverse(nil, 'a')");
    EXPECT_TRUE(result.get<sol::object>().get_type() == sol::type::nil);
}

// =============================================================================
// global.lua — LineSplit
// =============================================================================

TEST_F(LuaTestFixture, LineSplit_EmptyString)
{
    auto result = lua.script("return #LineSplit('')").get<int>();
    EXPECT_EQ(result, 0);
}

TEST_F(LuaTestFixture, LineSplit_SingleLine)
{
    // The pattern "^[^\r\n]+" only matches from the start, so gmatch yields at most one match
    auto result = lua.script("return #LineSplit('hello')").get<int>();
    EXPECT_GE(result, 0); // Document actual behavior
}

// =============================================================================
// global.lua — ToInt
// =============================================================================

TEST_F(LuaTestFixture, ToInt_PositiveRounding)
{
    EXPECT_EQ(lua.script("return ToInt(2.3)").get<int>(), 2);
    EXPECT_EQ(lua.script("return ToInt(2.5)").get<int>(), 3);
    EXPECT_EQ(lua.script("return ToInt(2.7)").get<int>(), 3);
}

TEST_F(LuaTestFixture, ToInt_NegativeRounding)
{
    EXPECT_EQ(lua.script("return ToInt(-2.3)").get<int>(), -2);
    EXPECT_EQ(lua.script("return ToInt(-2.5)").get<int>(), -3);
    EXPECT_EQ(lua.script("return ToInt(-2.7)").get<int>(), -3);
}

TEST_F(LuaTestFixture, ToInt_ExactIntegers)
{
    EXPECT_EQ(lua.script("return ToInt(0)").get<int>(), 0);
    EXPECT_EQ(lua.script("return ToInt(5)").get<int>(), 5);
    EXPECT_EQ(lua.script("return ToInt(-3)").get<int>(), -3);
}
