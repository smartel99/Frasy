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
local SyncRequirement  = require("lua/core/framework/sync_requirement")
local Sort             = require("lua/core/framework/sort_utils")
local Json             = require("lua/core/vendor/json")

Orchestrator           = {}

if Context.Orchestrator == nil then
    Context.Orchestrator = {
        sequences          = {},
        scope              = nil,
        solution           = {},
        values             = {},
        order_requirements = {},
        sync_requirements  = {}
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

function Orchestrator.RunSequence(sIndex, scope)
    local sequence             = Context.Orchestrator.sequences[scope.sequence]
    Context.Orchestrator.scope = scope

    local start                = os.clock()
    if sequence.time == nil then
        Context.Orchestrator.values[scope.sequence] = {}
        Log.d("Start sequence " .. scope.sequence)
        sequence.result.enabled = is_scope_enabled(scope)
        if sequence.result.enabled then
            local status, err = xpcall(sequence.func, error_handler)
            if not status then
                if type(err) == "string" then error(GenericError(err)) end
                if err.code ~= UnmetRequirement().code then error(err) end
                sequence.result.skipped = true
                sequence.result.reason  = err.what
            end
        else
            sequence.result.skipped = true
            sequence.result.reason  = "Disabled"
        end
    end

    if not sequence.result.skipped then
        for _, sqStage in ipairs(Context.Orchestrator.solution[sIndex]) do
            for _, sq in ipairs(sqStage) do
                if (sq.name == scope.sequence) then
                    for _, tStage in ipairs(sq.tests) do
                        for _, test in ipairs(tStage) do
                            Orchestrator.RunTest(Scope:new(scope.sequence, test))
                        end
                    end
                end
            end
        end

        sequence.result.pass = true
        local incomplete     = false
        for _, test in pairs(sequence.tests) do
            if test.result.time.stop == 0 then
                incomplete = true
            elseif not test.result.pass then
                sequence.result.pass = false
            end
        end
        if not incomplete then
            Log.i(string.format("Sequence %s: %s", scope.sequence, sequence.result.pass and "PASS" or "FAIL"))
        end
    elseif sequence.time == nil then
        Log.i(string.format("Sequence %s: SKIPPED. Reason: %s", scope.sequence, sequence.result.reason))
    else
        -- Nothing, we already warned that this sequence was skipped or disabled
    end

    if sequence.time == nil then
        sequence.time = { { start = start, stop = os.clock() } }
    else
        table.insert(sequence.time, { start = start, stop = os.clock() })
    end
end

function Orchestrator.RunTest(scope)
    local test                                              = Context.Orchestrator.sequences[scope.sequence].tests[scope.test]
    test.result.enabled                                     = is_scope_enabled(scope)
    Context.Orchestrator.values[scope.sequence][scope.test] = {}
    Context.Orchestrator.scope                              = scope
    Log.d("Start Test " .. scope.test)
    test.expectations      = {}
    test.result.time       = {}
    test.result.time.start = os.clock()
    if test.result.enabled then
        local status, err = xpcall(test.func, error_handler)
        if not status then
            Team.Fail()
            if type(err) == "string" then
                Team.Sync(test.result)
                error(GenericError(err))
            elseif err.code == UnmetExpectation().code then
                test.result.pass   = false
                test.result.reason = err.what
            elseif err.code == UnmetRequirement().code then
                test.result.skipped = true
                test.result.reason  = err.what
            else
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
    else
        test.result.skipped = true
        test.result.reason  = "Disabled"
    end
    test.result.time.stop    = os.clock()
    test.result.time.elapsed = test.result.time.stop - test.result.time.start -- Might change in the future
    test.result.time.process = test.result.time.stop - test.result.time.start
    Team.Sync(test.result)
    if test.result.skipped then
        Log.w(string.format("Test %s SKIPPED\r\nReason: %s", scope.test, test.result.reason))
    elseif not test.result.pass then
        Log.e(string.format("Test %s FAILED\r\nReason: %s", scope.test, test.result.reason))
    else
        Log.i(string.format("Test %s PASSED", scope.test))
    end
end

function Orchestrator.Generate()
    Log.d("Generation")
    local sd = {} -- sequences dependencies
    local td = {} -- tests dependencies
    local ss = {} -- sequences synchronization
    local ts = {} -- tests synchronization
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
            ts[sName]        = {}
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

    for _, requirement in pairs(Context.Orchestrator.sync_requirements) do
        if requirement.scope.sequence == nil then
            error(InvalidRequirement())
        end
        if not Orchestrator.HasScope(requirement.scope) then
            error(InvalidRequirement())
        end
        if requirement.scope.test ~= nil then
            ss[requirement.scope.sequence]                         = 0
            ts[requirement.scope.sequence][requirement.scope.test] = 0
        else
            ss[requirement.scope.sequence] = 0
        end
    end

    local ordered     = {}
    ordered.sequences = Sort.SortScopes(sn, edges.first.sequence, edges.last.sequence, sd)
    ordered.tests     = {}
    for sequence, tests in pairs(tn) do
        ordered.tests[sequence] = Sort.SortScopes(
                tests,
                edges.first.tests[sequence],
                edges.last.tests[sequence],
                td[sequence])
    end

    local sectionized     = {}
    sectionized.sequences = Sort.Sectionize(ordered.sequences, ss)
    sectionized.tests     = {}
    for sequence, tests in pairs(ordered.tests) do
        sectionized.tests[sequence] = Sort.Sectionize(tests, ts[sequence])
    end

    Context.Orchestrator.solution = Sort.CombineSectionized(sectionized.sequences, sectionized.tests)
    return Context.Orchestrator.solution
end

function Orchestrator.Validate()
    Log.d("Validation")
    for sIndex, section in ipairs(Context.Orchestrator.solution) do
        Context.Orchestrator.section = section
        for _, sequenceStage in ipairs(section) do
            for _, sequence in ipairs(sequenceStage) do
                Orchestrator.RunSequence(sIndex, Scope:new(sequence.name))
            end
        end
    end
end

function Orchestrator.ExecuteSection(index)
    Log.d("Section execution : " .. tostring(index))
    local section = Context.Orchestrator.solution[index]
    local results = { time = {} }
    if index == 1 then
        Context.Orchestrator.section         = {}
        Context.Orchestrator.section         = {}
        Context.Orchestrator.section.results = {}
        Context.time.start                   = os.clock()
        results.time.start                   = os.clock()
    end
    for _, sequenceStage in ipairs(section) do
        for _, sequence in ipairs(sequenceStage) do
            Orchestrator.RunSequence(index, Scope:new(sequence.name))
        end
    end
    results.time.stop                           = os.clock()
    Context.Orchestrator.section.results[index] = results
end

function Orchestrator.CompileExecutionResults(outputDir)
    local report     = {}
    report.info      = {
        version  = Context.version,
        operator = Context.operator,
        serial   = Context.serial,
        uut      = Context.uut,
        date     = os.date(),
        pass     = true,
        time     = {
            start   = Context.time.start,
            stop    = os.clock(),
            elapsed = os.clock() - Context.time.start,
            process = 0,
        }
    }
    report.sequences = {}
    for sName, sequence in pairs(Context.Orchestrator.sequences) do
        if not sequence.result.pass then
            report.info.pass = false
        end
        report.sequences[sName]       = sequence.result
        report.sequences[sName].tests = {}
        for tName, test in pairs(sequence.tests) do
            report.sequences[sName].tests[tName]              = test.result
            report.sequences[sName].tests[tName].expectations = { }
            for _, expectation in pairs(test.expectations) do
                table.insert(report.sequences[sName].tests[tName].expectations, expectation)
            end
        end
        report.sequences[sName].time       = { start = 0, stop = 0, elapsed = 0, process = 0 }
        report.sequences[sName].time.start = sequence.time[1].start
        for _, time in ipairs(sequence.time) do
            report.sequences[sName].time.stop    = time.stop
            report.sequences[sName].time.process = report.sequences[sName].time.process + time.stop - time.start
        end
        report.sequences[sName].time.elapsed = report.sequences[sName].time.stop - report.sequences[sName].time.start
        report.info.time.process            = report.info.time.process + report.sequences[sName].time.process
    end
    Utils.save_as_json(report, string.format("%s/%s.json", outputDir, Context.uut))
end

function Orchestrator.SaveSolution(path)
    Utils.save_as_json(Context.Orchestrator.solution, path)
end

function Orchestrator.LoadSolution(path)
    local file                    = io.open(path, "r")
    Context.Orchestrator.solution = Json.decode(file:read("*all"))
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

function Orchestrator.GetSyncRequirement()
    if not Orchestrator.IsInSequence() then error(BadScope()) end
    return SyncRequirement:new(Context.Orchestrator.scope)
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

function Orchestrator.AddSyncRequirement(requirement)
    if not Orchestrator.IsInSequence() then error(BadScope()) end
    table.insert(Context.Orchestrator.sync_requirements, requirement)
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
    if not Orchestrator.HasScope(scope) then error(BadScope("S: %s, T: %s", tostring(sequence),
                                                            tostring(test))) end
    local s = Context.Orchestrator.enable_list[sequence]
    if s == nil then s = { enabled = true } end
    if test ~= nil then s[test] = true end
    Context.Orchestrator.enable_list[sequence] = s
end

function Orchestrator.Disable(sequence, test)
    local scope = Scope:new(sequence, test)
    if not Orchestrator.HasScope(scope) then error(BadScope("S: %s, T: %s", tostring(sequence),
                                                            tostring(test))) end
    local s = Context.Orchestrator.enable_list[sequence]
    if s == nil then s = { enabled = true } end
    if test == nil then s = { enabled = false }
    else s[test] = false end
    Context.Orchestrator.enable_list[sequence] = s
end