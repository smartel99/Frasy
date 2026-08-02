#include "lua_test_fixture.h"

#include <gtest/gtest.h>

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
