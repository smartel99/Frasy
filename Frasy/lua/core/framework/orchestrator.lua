--- @file    orchestrator.lua
--- @author  Paul Thomas
--- @date    2023-03-15
--- @brief
---
--- @copyright
--- This program is free software: you can redistribute it and/or modify it under the
--- terms of the GNU General Public License as published by the Free Software Foundation, either
--- version 3 of the License, or (at your option) any later version.
--- This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
--- even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
--- General Public License for more details.
--- You should have received a copy of the GNU General Public License along with this program. If
--- not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.

local Scope            = require("lua/core/framework/scope")
local Sequence         = require("lua/core/framework/sequence")
local Test             = require("lua/core/framework/test")
local ScopeRequirement = require("lua/core/framework/scope_requirement/module")
local OrderRequirement = require("lua/core/framework/order_requirement")
local Sort             = require("lua/core/framework/sort_utils")

Orchestrator           = {}

if Context.Orchestrator == nil then
    Context.Orchestrator = {
        sequences          = {},
        scope              = nil,
        order              = {},
        values             = {},
        order_requirements = {},
    }
end

local function error_handler(err)
    if type(err) == 'string' then err = GenericError(err) end
    err.what = err.what .. "\r\n" .. debug.traceback(nil, 2)
    return err
end

local function is_scope_enabled(scope)
    if not Orchestrator.HasScope(scope) then error(NotFound()) end
    local sequence = Context.Orchestrator.enable_list[scope.sequence]
    if sequence == nil then return true end
    if scope.test == nil then
        return sequence.enabled == true
    else
        if sequence.enabled == false then return false end
        local test = Context.Orchestrator.enable_list[scope.sequence][scope.test]
        return test.enabled == true
    end
end

function Orchestrator.Generate()
    local sd = {} -- sequences dependencies
    local td = {} -- tests dependencies
    local sn = {} -- sequence names
    local tn = {} -- test names
    for sName, sequence in pairs(Context.Orchestrator.sequences) do
        Context.Orchestrator.scope = Scope:new(sName)
        sequence.func()
        sd[sName] = {}
        td[sName] = {}
        table.insert(sn, sName)
        tn[sName] = {}
        for tName, test in pairs(sequence.tests) do
            Context.Orchestrator.scope = Scope:new(sName, tName)
            test.func()
            td[sName][tName] = {}
            table.insert(tn[sName], tName)
        end
    end
    Context.Orchestrator.scope = nil
    local edges                = {
        first = {
            sequence = nil,
            tests    = {},
        },
        last  = {
            sequence = nil,
            tests    = {},
        },
    }

    for _, requirement in ipairs(Context.Orchestrator.order_requirements) do
        if not Orchestrator.HasScope(requirement.scope) then
            error(InvalidRequirement())
        end
        if requirement.kind == OrderRequirement.Kind.First then
            Sort.AddEdgeRequirement(edges.first, requirement)
        elseif requirement.kind == OrderRequirement.Kind.Last then
            Sort.AddEdgeRequirement(edges.last, requirement)
        elseif requirement.kind == OrderRequirement.Kind.After then
            if requirement.reference == nil then
                error(InvalidRequirement())
            end
            if requirement.reference.sequence == nil then
                error(InvalidRequirement())
            end
            if requirement.scope.sequence == nil then
                error(InvalidRequirement())
            end
            if not Orchestrator.HasScope(requirement.reference) then
                error(InvalidRequirement())
            end
            if requirement.scope.sequence == requirement.reference.sequence then
                if requirement.scope.test == nil then
                    error(InvalidRequirement())
                end
                if requirement.reference.test == nil then
                    error(InvalidRequirement())
                end
                td[requirement.scope.sequence][requirement.scope.test][requirement.reference.test] = 0
            else
                sd[requirement.scope.sequence][requirement.reference.sequence] = 0
            end
        else
            error(InvalidRequirement())
        end
    end

    Context.Orchestrator.order.sequences = Sort.SortScopes(sn, edges.first.sequence, edges.last.sequence, sd)
    Context.Orchestrator.order.tests     = {}
    for sequence, tests in pairs(tn) do
        Context.Orchestrator.order.tests[sequence] = Sort.SortScopes(
                tests,
                edges.first.tests[sequence],
                edges.last.tests[sequence],
                td[sequence])
    end

    return Context.Orchestrator.order
end

function Orchestrator.Validate()
    for _, sLayer in ipairs(Context.Orchestrator.order.sequences) do
        for _, sequence in ipairs(sLayer) do
            Context.Orchestrator.scope            = Scope:new(sequence)
            Context.Orchestrator.values[sequence] = {}
            if not Orchestrator.HasSequence(Context.Orchestrator.scope) then
                error(NotFound())
            end
            Context.Orchestrator.sequences[sequence]:func()
            for _, tLayer in ipairs(Context.Orchestrator.order.tests[sequence]) do
                for _, test in ipairs(tLayer) do
                    Context.Orchestrator.scope                  = Scope:new(sequence, test)
                    Context.Orchestrator.values[sequence][test] = {}
                    if not Orchestrator.HasTest(Context.Orchestrator.scope) then
                        error(NotFound())
                    end
                    Log.d(Context.uut .. ": " .. sequence .. "-" .. test)
                    Context.Orchestrator.sequences[sequence].tests[test].func()
                    Log.i(Context.uut .. ": " .. sequence .. "-" .. test .. " OK!")
                end
            end
        end
    end
end

function Orchestrator.ExecuteSequence(scope)
    local sequence                              = Context.Orchestrator.sequences[scope.sequence]
    Context.Orchestrator.values[scope.sequence] = {}
    Context.Orchestrator.scope                  = scope
    Log.d("Start sequence " .. scope.sequence)
    sequence.result.enabled = is_scope_enabled(scope)
    sequence.result.start   = os.clock()
    local status, err       = xpcall(sequence.func, error_handler)
    if not status then
        if type(err) == "string" then error(GenericError(err)) end
        if err.code ~= UnmetRequirement().code then error(err) end
        sequence.result.skipped = true
    elseif not sequence.result.enabled then
        sequence.result.skipped = true
    end

    for _, layer in ipairs(Context.Orchestrator.order.tests[scope.sequence]) do
        for _, test in ipairs(layer) do
            Orchestrator.ExecuteTest(Scope:new(scope.sequence, test))
        end
    end

    if not sequence.result.skipped then
        sequence.result.pass = true
        for _, test in pairs(sequence.tests) do
            if not test.result.pass then
                sequence.result.pass = false
            end
        end
        Log.i(string.format(
                "Sequence %s: %s",
                scope.sequence, sequence.result.pass and "PASS" or "FAIL"))
    else
        if sequence.result.enabled then
            Log.i(string.format("Sequence %s: DISABLED", scope.sequence))
        else
            Log.i(string.format("Sequence %s: SKIPPED", scope.sequence))
        end
    end
    sequence.result.stop = os.clock()
end

function Orchestrator.ExecuteTest(scope)
    local test                                              = Context.Orchestrator.sequences[scope.sequence].tests[scope.test]
    test.result.enabled                                     = is_scope_enabled(scope)
    Context.Orchestrator.values[scope.sequence][scope.test] = {}
    Context.Orchestrator.scope                              = scope
    Log.d("Start Test " .. scope.test)
    test.expectations = {}
    test.result.start = os.clock()
    local status, err = xpcall(test.func, error_handler)
    if not status then
        if type(err) == "string" then
            Team.Sync(test.result)
            error(GenericError(err))
        elseif err.code == UnmetExpectation().code or err.code == TeamError().code then
            test.result.pass   = false
            test.result.reason = err.what
            Team.Fail()
        elseif err.code == UnmetRequirement().code then
            test.result.skipped = true
            test.result.reason  = err.what
            Team.Fail()
        else
            Utils.print(err)
            Team.Sync(test.result)
            error(err)
        end
    else
        test.result.pass = true
        for _, expectation in ipairs(test.expectations) do
            if expectation.pass == expectation.inverted then
                test.result.pass   = false
                test.result.reason = "Unmet expectation"
            end
        end
    end
    test.result.stop = os.clock()
    Team.Sync(test.result)
    if test.result.skipped then
        Log.w(string.format("Test %s SKIPPED\r\nReason: %s", scope.test, test.result.reason))
    elseif not test.result.pass then
        Log.e(string.format("Test %s FAILED\r\nReason: %s", scope.test, test.result.reason))
    else
        Log.i(string.format("Test %s PASSED", scope.test))
    end
end

function Orchestrator.Execute()
    local start               = os.clock()
    Context.Orchestrator.date = os.date()
    for _, sLayer in ipairs(Context.Orchestrator.order.sequences) do
        for _, sequence in ipairs(sLayer) do
            Orchestrator.ExecuteSequence(Scope:new(sequence))
        end
    end

    local results = { sequences = {}, pass = true }
    for sName, sequence in pairs(Context.Orchestrator.sequences) do
        if not sequence.result.pass then results.pass = false end
        results.sequences[sName]       = sequence.result
        results.sequences[sName].tests = {}
        for tName, test in pairs(sequence.tests) do
            results.sequences[sName].tests[tName]              = test.result
            results.sequences[sName].tests[tName].expectations = { }
            for _, expectation in pairs(test.expectations) do
                table.insert(results.sequences[sName].tests[tName].expectations, expectation)
            end
        end
    end
    local stop       = os.clock()
    results.date     = Context.Orchestrator.date
    results.duration = stop - start
    return results
end

function Orchestrator.SetOrder(order)
    Context.Orchestrator.order = order
end

function Orchestrator.CreateSequence(name, func)
    if Orchestrator.IsInSequence() then error(NestedScope()) end
    if Orchestrator.HasSequence(Scope:new(name)) then error(AlreadyDefined()) end
    Context.Orchestrator.sequences[name] = Sequence:new(func)
end

function Orchestrator.CreateTest(name, func)
    if not Orchestrator.IsInSequence() then error(BadScope()) end
    if Orchestrator.IsInTest() then error(NestedScope()) end
    Context.Orchestrator.sequences[Context.Orchestrator.scope.sequence].tests[name] = Test:new(func)
end

function Orchestrator.GetSequenceScopeRequirement(name)
    if not Orchestrator.IsInSequence() then error(BadScope()) end
    if name == nil then name = Context.Orchestrator.scope.sequence end
    return ScopeRequirement:new(Orchestrator, Scope:new(name, nil))
end

function Orchestrator.GetTestScopeRequirement(name)
    if not Orchestrator.IsInSequence() then error(BadScope()) end
    if name == nil then
        if not Orchestrator.IsInTest() then error(BadScope()) end
        name = Context.Orchestrator.scope.test
    end
    return ScopeRequirement:new(Orchestrator, Scope:new(Context.Orchestrator.scope.sequence, name))
end

function Orchestrator.IsInSequence() return Context.Orchestrator.scope ~= nil end

function Orchestrator.IsInTest()
    return Context.Orchestrator.scope ~= nil and Context.Orchestrator.scope.test ~= nil
end

function Orchestrator.HasSequence(scope) return Context.Orchestrator.sequences[scope.sequence] ~= nil end

function Orchestrator.HasTest(scope)
    return Orchestrator.HasSequence(scope) and Context.Orchestrator.sequences[scope.sequence].tests[scope.test] ~= nil
end

function Orchestrator.HasScope(scope)
    return scope.test == nil and Orchestrator.HasSequence(scope) or Orchestrator.HasTest(scope)
end

function Orchestrator.HasValue(scope, name)
    if not Orchestrator.HasScope(scope) then error(BadScope()) end
    return Context.Orchestrator.values[scope.sequence][scope.test][name] ~= nil
end

function Orchestrator.SetValue(scope, name, value)
    if Orchestrator.HasValue(scope, name) then error(AlreadyDefined()) end
    Context.Orchestrator.values[scope.sequence][scope.test][name] = value
end

function Orchestrator.GetValue(scope, name)
    if not Orchestrator.HasValue(scope, name) then error(NotFound()) end
    return Context.Orchestrator.values[scope.sequence][scope.test][name]
end

function Orchestrator.AddOrderRequirement(requirement)
    if not Orchestrator.IsInSequence() then error(BadScope()) end
    table.insert(Context.Orchestrator.order_requirements, requirement)
end

function Orchestrator.AddExpectationResult(result)
    if not Orchestrator.IsInTest() then error(BadScope()) end
    local scope = Context.Orchestrator.scope
    table.insert(Context.Orchestrator.sequences[scope.sequence].tests[scope.test].expectations, result)
end

function Orchestrator.GetScope() return Context.Orchestrator.scope end

function Orchestrator.HasPassed(scope)
    if not Orchestrator.HasScope(scope) then error(NotFound(scope:ToString())) end
    local s = Context.Orchestrator.sequences[scope.sequence]
    if scope.test ~= nil then s = s.tests[scope.test] end
    return (not s.result.skipped) and s.result.pass
end

function Orchestrator.HasBeenSkipped(scope)
    if not Orchestrator.HasScope(scope) then error(NotFound(scope:ToString())) end
    local s = Context.Orchestrator.sequences[scope.sequence]
    if scope.test ~= nil then s = s.tests[scope.test] end
    return s.result.skipped
end

function Orchestrator.Enable(sequence, test)
    local scope = Scope:new(sequence, test)
    if not Orchestrator.HasScope(scope) then error(BadScope("S: %s, T: %s", tostring(sequence), tostring(test))) end
    local s = Context.Orchestrator.enable_list[sequence]
    if s == nil then s = { enabled = true } end
    if test ~= nil then s[test] = true end
    Context.Orchestrator.enable_list[sequence] = s
end

function Orchestrator.Disable(sequence, test)
    local scope = Scope:new(sequence, test)
    if not Orchestrator.HasScope(scope) then error(BadScope("S: %s, T: %s", tostring(sequence), tostring(test))) end
    local s = Context.Orchestrator.enable_list[sequence]
    if s == nil then s = { enabled = true } end
    if test == nil then s = { enabled = false }
    else s[test] = false end
    Context.Orchestrator.enable_list[sequence] = s
end