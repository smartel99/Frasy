/**
 * @file    execution_fixture.h
 * @brief   Fixture that defines sequences and tests at low level and runs them through ExecuteSection.
 */

#ifndef FRASY_TESTS_ORCHESTRATOR_EXECUTION_FIXTURE_H
#define FRASY_TESTS_ORCHESTRATOR_EXECUTION_FIXTURE_H

#include "orchestrator_test_fixture.h"

#include <string>

class ExecutionTest : public OrchestratorTestFixture {
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        // Execution times scopes with os.clock().
        lua.open_libraries(sol::lib::os);
        // Define({ { name, func?, tests = { { name, func? }, ... }, section? }, ... })
        // Tests of a sequence are put in the section given by their `section` field (default 1).
        lua.script(R"(
            local Scope = require('lua/core/framework/scope')
            ran = {}
            function Define(sequences)
                local solution = {}
                for _, s in ipairs(sequences) do
                    Context.orchestrator.scope = nil
                    Orchestrator.CreateSequence(s.name, s.func or function() end, 'test', 0)
                    Context.orchestrator.scope = Scope:New(s.name)
                    local bySection = {}
                    for _, t in ipairs(s.tests) do
                        local func = t.func or function() end
                        Orchestrator.CreateTest(t.name, function()
                            ran[s.name .. '.' .. t.name] = true
                            func()
                        end, 'test', 0)
                        local section = t.section or 1
                        bySection[section] = bySection[section] or {}
                        table.insert(bySection[section], t.name)
                    end
                    for section, names in pairs(bySection) do
                        solution[section] = solution[section] or { {} }
                        table.insert(solution[section][1], { name = s.name, tests = { names } })
                    end
                end
                Context.orchestrator.scope = nil
                Context.orchestrator.solution = solution
                Context.info.stage = Stage.execution
            end
            function ResultOf(sequence, test)
                local s = Context.orchestrator.sequences[sequence]
                if test ~= nil then s = s.tests[test] end
                return s.result
            end
            function Fail() Orchestrator.AddExpectationResult({ name = 'E', pass = false }) end
        )");
    }

    std::string lastError;

    bool runSection(int index = 1)
    {
        auto run =
          lua.safe_script("Orchestrator.ExecuteSection(" + std::to_string(index) + ")", sol::script_pass_on_error);
        if (!run.valid()) {
            sol::error err = run;
            lastError      = err.what();
        }
        return run.valid();
    }

    std::string result(const std::string& sequence, const std::string& test = "")
    {
        return lua.script("return ResultOf('" + sequence + "'" + (test.empty() ? "" : ", '" + test + "'") + ").result")
          .get<std::string>();
    }

    std::string reason(const std::string& sequence, const std::string& test = "")
    {
        return lua
          .script("return tostring(ResultOf('" + sequence + "'" + (test.empty() ? "" : ", '" + test + "'") +
                  ").reason)")
          .get<std::string>();
    }

    bool field(const std::string& sequence, const std::string& test, const std::string& name)
    {
        return lua.script("return ResultOf('" + sequence + "', '" + test + "')." + name).get<bool>();
    }

    bool ran(const std::string& sequence, const std::string& test)
    {
        return lua.script("return ran['" + sequence + "." + test + "'] == true").get<bool>();
    }

    bool query(const std::string& function, const std::string& sequence, const std::string& test = "")
    {
        return lua
          .script("return Orchestrator." + function + "(require('lua/core/framework/scope'):New('" + sequence + "'" +
                  (test.empty() ? "" : ", '" + test + "'") + "))")
          .get<bool>();
    }
};

#endif    // FRASY_TESTS_ORCHESTRATOR_EXECUTION_FIXTURE_H
