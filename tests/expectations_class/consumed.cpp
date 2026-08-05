/**
 * @file    consumed.cpp
 * @author  Paul Thomas
 * @date    2026-08-04
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <https://www.gnu.org/licenses/>.
 */

#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class ExpectationConsumedTest : public OrchestratorTestFixture {
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

TEST_F(ExpectationConsumedTest, Consumed)
{
    sol::protected_function run = lua.script(R"(return function()
            local expectation = Expectation:New(true, "")
            expectation:ToBeTrue()
            expectation:ToBeTrue()
        end)");
    run.set_error_handler(lua.script_file("lua/core/framework/error_handler.lua"));
    sol::protected_function_result result = run();
    EXPECT_FALSE(result.valid());
    std::string error = result.get<sol::error>().what();
    EXPECT_NE(error.find("ConsumedExpectation"), std::string::npos);
}