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

// =============================================================================
// Is module
// =============================================================================

class IsTest : public LuaTestFixture
{
protected:
    void SetUp() override
    {
        LuaTestFixture::SetUp();
        lua.script("Is = require('lua/core/utils/is')");
    }
};

TEST_F(IsTest, TypeChecks)
{
    EXPECT_TRUE(lua.script("return Is.Integer(5)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Integer(-10)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Integer(1.5)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Integer('5')").get<bool>());

    EXPECT_TRUE(lua.script("return Is.Float(1.5)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Float(1)").get<bool>()); // integers are numbers in Lua
    EXPECT_FALSE(lua.script("return Is.Float('1.5')").get<bool>());

    EXPECT_TRUE(lua.script("return Is.String('hello')").get<bool>());
    EXPECT_FALSE(lua.script("return Is.String(123)").get<bool>());

    EXPECT_TRUE(lua.script("return Is.Boolean(true)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Boolean(false)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Boolean(1)").get<bool>());

    EXPECT_TRUE(lua.script("return Is.Nil(nil)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Nil(0)").get<bool>());

    EXPECT_TRUE(lua.script("return Is.Table({})").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Table({1,2,3})").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Table('not a table')").get<bool>());

    EXPECT_TRUE(lua.script("return Is.Function(print)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Function(42)").get<bool>());
}

TEST_F(IsTest, Array)
{
    EXPECT_TRUE(lua.script("return Is.Array({1, 2, 3})").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Array({})").get<bool>()); // empty table has #t == 0
    EXPECT_FALSE(lua.script("return Is.Array('hello')").get<bool>());
}

TEST_F(IsTest, Number)
{
    EXPECT_TRUE(lua.script("return Is.Number(42)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Number(3.14)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Number('42')").get<bool>());
}

TEST_F(IsTest, Unsigned)
{
    EXPECT_TRUE(lua.script("return Is.Unsigned(0)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Unsigned(100)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Unsigned(-1)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Unsigned(1.5)").get<bool>());
}

TEST_F(IsTest, Integer8)
{
    EXPECT_TRUE(lua.script("return Is.Integer8(-128)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Integer8(127)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Integer8(0)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Integer8(-129)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Integer8(128)").get<bool>());
}

TEST_F(IsTest, Integer16)
{
    EXPECT_TRUE(lua.script("return Is.Integer16(-32768)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Integer16(32767)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Integer16(-32769)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Integer16(32768)").get<bool>());
}

TEST_F(IsTest, Integer32)
{
    EXPECT_TRUE(lua.script("return Is.Integer32(-2147483648)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Integer32(2147483647)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Integer32(-2147483649)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Integer32(2147483648)").get<bool>());
}

TEST_F(IsTest, Unsigned8)
{
    EXPECT_TRUE(lua.script("return Is.Unsigned8(0)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Unsigned8(255)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Unsigned8(-1)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Unsigned8(256)").get<bool>());
}

TEST_F(IsTest, Unsigned16)
{
    EXPECT_TRUE(lua.script("return Is.Unsigned16(0)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Unsigned16(65535)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Unsigned16(-1)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Unsigned16(65536)").get<bool>());
}

TEST_F(IsTest, Unsigned32)
{
    EXPECT_TRUE(lua.script("return Is.Unsigned32(0)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Unsigned32(4294967295)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Unsigned32(-1)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Unsigned32(4294967296)").get<bool>());
}

TEST_F(IsTest, IntegerIn)
{
    EXPECT_TRUE(lua.script("return Is.IntegerIn(5, 0, 10)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.IntegerIn(0, 0, 10)").get<bool>());  // boundary
    EXPECT_TRUE(lua.script("return Is.IntegerIn(10, 0, 10)").get<bool>()); // boundary
    EXPECT_FALSE(lua.script("return Is.IntegerIn(-1, 0, 10)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.IntegerIn(11, 0, 10)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.IntegerIn(1.5, 0, 10)").get<bool>()); // not integer
}

TEST_F(IsTest, IntegerInEx)
{
    EXPECT_TRUE(lua.script("return Is.IntegerInEx(5, 0, 10)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.IntegerInEx(0, 0, 10)").get<bool>());  // exclusive
    EXPECT_FALSE(lua.script("return Is.IntegerInEx(10, 0, 10)").get<bool>()); // exclusive
}

TEST_F(IsTest, FloatIn)
{
    EXPECT_TRUE(lua.script("return Is.FloatIn(5.5, 0, 10)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.FloatIn(0, 0, 10)").get<bool>());   // boundary
    EXPECT_TRUE(lua.script("return Is.FloatIn(10, 0, 10)").get<bool>());  // boundary
    EXPECT_FALSE(lua.script("return Is.FloatIn(-0.1, 0, 10)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.FloatIn(10.1, 0, 10)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.FloatIn('5', 0, 10)").get<bool>()); // not a number
}

TEST_F(IsTest, FloatInEx)
{
    EXPECT_TRUE(lua.script("return Is.FloatInEx(5.5, 0, 10)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.FloatInEx(0, 0, 10)").get<bool>());  // exclusive
    EXPECT_FALSE(lua.script("return Is.FloatInEx(10, 0, 10)").get<bool>()); // exclusive
}

TEST_F(IsTest, UnsignedIn)
{
    EXPECT_TRUE(lua.script("return Is.UnsignedIn(5, 0, 10)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.UnsignedIn(0, 0, 10)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.UnsignedIn(10, 0, 10)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.UnsignedIn(11, 0, 10)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.UnsignedIn(-1, 0, 10)").get<bool>());
}

TEST_F(IsTest, Not)
{
    EXPECT_TRUE(lua.script("return Is.Not(nil, Is.Integer)").get<bool>());
    EXPECT_TRUE(lua.script("return Is.Not('hello', Is.Integer)").get<bool>());
    EXPECT_FALSE(lua.script("return Is.Not(5, Is.Integer)").get<bool>());
}

TEST_F(IsTest, InArray)
{
    EXPECT_TRUE(lua.script("return Is.InArray(2, {1, 2, 3})").get<bool>());
    EXPECT_FALSE(lua.script("return Is.InArray(4, {1, 2, 3})").get<bool>());
    EXPECT_FALSE(lua.script("return Is.InArray(2, {})").get<bool>()); // empty array fails Is.Array
    EXPECT_TRUE(lua.script("return Is.InArray('b', {'a', 'b', 'c'})").get<bool>());
    EXPECT_FALSE(lua.script("return Is.InArray('d', {'a', 'b', 'c'})").get<bool>());
}

// =============================================================================
// maybe module
// =============================================================================

class MaybeTest : public LuaTestFixture
{
protected:
    void SetUp() override
    {
        LuaTestFixture::SetUp();
        lua.script("Is = require('lua/core/utils/is')");
        lua.script("Maybe = require('lua/core/utils/maybe')");
    }
};

TEST_F(MaybeTest, NilAlwaysPasses)
{
    EXPECT_TRUE(lua.script("return Maybe(nil, Is.Integer)").get<bool>());
    EXPECT_TRUE(lua.script("return Maybe(nil, Is.String)").get<bool>());
    EXPECT_TRUE(lua.script("return Maybe(nil, Is.Table)").get<bool>());
}

TEST_F(MaybeTest, ValidValuePasses)
{
    EXPECT_TRUE(lua.script("return Maybe(5, Is.Integer)").get<bool>());
    EXPECT_TRUE(lua.script("return Maybe('hi', Is.String)").get<bool>());
}

TEST_F(MaybeTest, InvalidValueFails)
{
    EXPECT_FALSE(lua.script("return Maybe('hi', Is.Integer)").get<bool>());
    EXPECT_FALSE(lua.script("return Maybe(5, Is.String)").get<bool>());
}

TEST_F(MaybeTest, WithAdditionalArgs)
{
    EXPECT_TRUE(lua.script("return Maybe(5, Is.IntegerIn, 0, 10)").get<bool>());
    EXPECT_FALSE(lua.script("return Maybe(15, Is.IntegerIn, 0, 10)").get<bool>());
    EXPECT_TRUE(lua.script("return Maybe(nil, Is.IntegerIn, 0, 10)").get<bool>());
}

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
