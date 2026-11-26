#include "orchestrator_test_fixture.h"
#include "utils/lua/expectation.h"

#include <gtest/gtest.h>
#include <sol/sol.hpp>

#include <vector>

// =============================================================================
// Expectation::fromTable — verdict shown by the live expectation views
// =============================================================================

class ExpectationFromTableTest : public ::testing::Test {
protected:
    sol::state lua;

    void SetUp() override { lua.open_libraries(sol::lib::base); }

    Frasy::Lua::Expectation fromLua(const std::string& table)
    {
        return Frasy::Lua::Expectation::fromTable(lua.script("return " + table).get<sol::table>());
    }
};

TEST_F(ExpectationFromTableTest, ReadsResult)
{
    EXPECT_TRUE(fromLua("{ name = 'x', method = 'ToBeEqual', value = 1, expected = 1, result = 'pass' }").pass);
    EXPECT_FALSE(fromLua("{ name = 'x', method = 'ToBeEqual', value = 1, expected = 2, result = 'fail' }").pass);
}

TEST_F(ExpectationFromTableTest, InvertedIsNotAppliedTwice)
{
    // An inverted, unmet expectation passes; Lua already wrote that into `result` and `pass`.
    auto expectation = fromLua(
      "{ name = 'x', method = 'ToBeEqual', value = 1, expected = 2, inverted = true, result = 'pass', pass = true }");
    EXPECT_TRUE(expectation.pass);
    expectation = fromLua(
      "{ name = 'x', method = 'ToBeEqual', value = 1, expected = 1, inverted = true, result = 'fail', pass = false }");
    EXPECT_FALSE(expectation.pass);
}

TEST_F(ExpectationFromTableTest, ResultWinsOverPass)
{
    EXPECT_FALSE(fromLua("{ name = 'x', method = 'ToBeTrue', value = true, result = 'fail', pass = true }").pass);
}

TEST_F(ExpectationFromTableTest, FallsBackToPassWithoutResult)
{
    EXPECT_TRUE(fromLua("{ name = 'x', method = 'ToBeTrue', value = true, pass = true }").pass);
    EXPECT_FALSE(fromLua("{ name = 'x', method = 'ToBeTrue', value = true, pass = false }").pass);
    EXPECT_FALSE(fromLua("{ name = 'x', method = 'ToBeTrue', value = true }").pass);
}

TEST_F(ExpectationFromTableTest, UnknownResultFallsBackToPass)
{
    EXPECT_TRUE(fromLua("{ name = 'x', method = 'ToBeTrue', value = true, result = 'bogus', pass = true }").pass);
}

// =============================================================================
// Through the real Lua Expectation and ShowExpectation
// =============================================================================

class ShownExpectationTest : public OrchestratorTestFixture {
protected:
    std::vector<Frasy::Lua::Expectation> m_shown;

    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        createTest("Seq", "T1", true);
        setStage("execution");
        lua.set_function("ShowExpectation", [this](const sol::table& table) {
            m_shown.push_back(Frasy::Lua::Expectation::fromTable(table));
        });
        lua.script("Expectation = require('lua/core/framework/expectation/expectation')");
        lua.script_file(getLuaBaseDir() + "/lua/core/framework/expectation/utils.lua");
    }
};

TEST_F(ShownExpectationTest, VerdictMatchesLua)
{
    lua.script(R"(
        Expectation:New(1, 'met'):ToBeEqual(1):Show()
        Expectation:New(1, 'unmet'):ToBeEqual(2):Show()
        Expectation:New(1, 'invertedMet', { inverted = true }):ToBeEqual(1):Show()
        Expectation:New(1, 'invertedUnmet', { inverted = true }):ToBeEqual(2):Show()
    )");
    ASSERT_EQ(m_shown.size(), 4U);
    EXPECT_TRUE(m_shown[0].pass);
    EXPECT_FALSE(m_shown[1].pass);
    EXPECT_FALSE(m_shown[2].pass);
    EXPECT_TRUE(m_shown[3].pass);
}
