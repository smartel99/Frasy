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
local Scope = require("lua/core/framework/scope")
local Sequence = require("lua/core/framework/sequence")
local Test = require("lua/core/framework/test")
local ScopeRequirement = require("lua/core/framework/scope_requirement/module")
local OrderRequirement = require("lua/core/framework/order_requirement")
local SyncRequirement = require("lua/core/framework/sync_requirement")
local Sort = require("lua/core/framework/sort_utils")
local Json = require("lua/core/vendor/json")

---@class Orchestrator
Orchestrator = {}

if Context.orchestrator == nil then
    Context.orchestrator = {
        sequences = {},
        scope = nil,
        solution = {},
        values = {},
        order_requirements = {},
        sync_requirements = {}
    }
end

local function ErrorHandler(err)
    if type(err) == "string" then
        err = GenericError(err)
    end
    err.what = err.what .. "\r\n" .. debug.traceback(nil, 2)
    return err
end

local function IsScopeEnabled(scope)
    if not Orchestrator.HasScope(scope) then
        error(NotFound())
    end
    local sequence = Context.orchestrator.enable_list[scope.sequence]
    if sequence == nil then
        return true
    end
    if scope.test == nil then
        return sequence.enabled == true
    else
        if sequence.enabled == false then
            return false
        end
        local test = Context.orchestrator.enable_list[scope.sequence][scope.test]
        return test.enabled == true
    end
end

function Orchestrator.RunSequence(sIndex, scope)
    local sequence = Context.orchestrator.sequences[scope.sequence]
    Context.orchestrator.scope = scope
    __profileStartEvent(scope.sequence, sequence.source, sequence.line)

    local start = os.clock()

    local reportEnd = function()
        if sequence.time == nil then
            sequence.time = { { start = start, stop = os.clock() } }
        else
            table.insert(sequence.time, { start = start, stop = os.clock() })
        end
    end

    if sequence.time == nil then
        Context.orchestrator.values[scope.sequence] = {}
        Log.D("Start sequence: " .. scope.sequence)
        sequence.result.enabled = IsScopeEnabled(scope)
        if sequence.result.enabled then
            local status, err = xpcall(sequence.func, ErrorHandler)
            if not status then
                if type(err) == "string" then
                    sequence.result.pass = false
                    sequence.result.reason = err
                    reportEnd()
                    error(GenericError(err))
                end
                if err.code ~= UnmetRequirement().code then
                    sequence.result.pass = false
                    sequence.result.reason = err.what
                    reportEnd()
                    error(err)
                end
                sequence.result.skipped = true
                sequence.result.reason = err.what
            end
        else
            sequence.result.skipped = true
            sequence.result.reason = "Disabled"
        end
    end

    if not sequence.result.skipped then
        for _, sqStage in ipairs(Context.orchestrator.solution[sIndex]) do
            for _, sq in ipairs(sqStage) do
                if (sq.name == scope.sequence) then
                    for _, tStage in ipairs(sq.tests) do
                        for _, test in ipairs(tStage) do
                            Orchestrator.RunTest(Scope:New(scope.sequence, test))
                        end
                    end
                end
            end
        end

        sequence.result.pass = true
        local incomplete = false
        for _, test in pairs(sequence.tests) do
            if test.result.time.stop == 0 then
                incomplete = true
            elseif not test.result.pass then
                sequence.result.pass = false
            end
        end
        if not incomplete then
            Log.I(string.format("Sequence %s: %s", scope.sequence,
                sequence.result.pass and "PASS" or "FAIL"))
        end
    elseif sequence.time == nil then
        Log.I(string.format("Sequence %s: SKIPPED. Reason: %s", scope.sequence,
            sequence.result.reason))
    else
        -- Nothing, we already warned that this sequence was skipped or disabled
    end

    reportEnd()

    __profileEndEvent(scope.sequence, sequence.source, sequence.line)
end

function Orchestrator.RunTest(scope)
    local test = Context.orchestrator.sequences[scope.sequence].tests[scope.test]
    test.result.enabled = IsScopeEnabled(scope)
    Context.orchestrator.values[scope.sequence][scope.test] = {}
    Context.orchestrator.scope = scope
    Log.D("Start test: " .. scope.test)
    test.expectations = {}
    test.result.time = {}
    test.result.time.start = os.clock()

    local reportEnd = function()
        test.result.time.stop = os.clock()
        test.result.time.elapsed = test.result.time.stop - test.result.time.start -- Might change in the future
        test.result.time.process = test.result.time.stop - test.result.time.start
    end

    __profileStartEvent(scope.test, test.source, test.line)
    if test.result.enabled then
        local status, err = xpcall(test.func, ErrorHandler)
        if not status then
            Team.Fail()
            if type(err) == "string" then
                test.result.pass = false
                test.result.reason = err
                reportEnd()
                Team.Sync(test.result)
                error(GenericError(err))
            elseif err.code == UnmetExpectation().code then
                test.result.pass = false
                test.result.reason = err.what
            elseif err.code == UnmetRequirement().code then
                test.result.skipped = true
                test.result.reason = err.what
            else
                test.result.pass = false
                test.result.reason = err
                reportEnd()
                Team.Sync(test.result)
                error(err)
            end
        else
            test.result.pass = true
            for _, expectation in ipairs(test.expectations) do
                if expectation.pass == expectation.inverted then
                    test.result.pass = false
                    test.result.reason = "Unmet expectation"
                end
            end
        end
    else
        test.result.skipped = true
        test.result.reason = "Disabled"
    end
    __profileEndEvent(scope.test, test.source, test.line)
    reportEnd()
    Team.Sync(test.result)
    if test.result.skipped then
        Log.W(string.format("Test %s SKIPPED\r\nReason: %s", scope.test,
            test.result.reason))
    elseif not test.result.pass then
        Log.E(string.format("Test %s FAILED\r\nReason: %s", scope.test,
            test.result.reason))
    else
        Log.I(string.format("Test %s PASSED", scope.test))
    end
end

function Orchestrator.Generate()
    Log.D("Generation")
    local sd = {} -- sequences dependencies
    local td = {} -- tests dependencies
    local ss = {} -- sequences synchronization
    local ts = {} -- tests synchronization
    local sn = {} -- sequence names
    local tn = {} -- test names
    local hasFailedSequences = true
    local completedSequences = {}
    while hasFailedSequences do
        hasFailedSequences = false
        local hasProgressOnSequences = false
        hasSequences = false
        for _, __ in pairs(Context.orchestrator.sequences) do
            hasSequences = true
            break
        end
        if not hasSequences then
            error(GenerationError("No sequences were found!"))
        end
        for sName, sequence in pairs(Context.orchestrator.sequences) do
            if completedSequences[sName] ~= nil then
                Log.D("Sequence " .. sName .. " already generated, skipping")
            else
                Context.orchestrator.scope = Scope:New(sName)
                Log.D("Generating sequence " .. sName)
                sequence.func()
                sd[sName] = {}
                td[sName] = {}
                tn[sName] = {}
                local _, err = xpcall(function()
                    local hasFailedTests = true
                    local completedTests = {}
                    while hasFailedTests do
                        hasFailedTests = false
                        local hasProgressOnTests = false
                        for tName, test in pairs(sequence.tests) do
                            if completedTests[tName] ~= nil then
                                Log.D("Test " .. tName ..
                                    " already generated, skipping")
                            else
                                Log.D("Generating test " .. tName)
                                Context.orchestrator.scope = Scope:New(sName,
                                    tName)
                                local _, err = xpcall(test.func, ErrorHandler)
                                if err == nil then
                                    td[sName][tName] = {}
                                    ts[sName] = {}
                                    table.insert(tn[sName], tName)
                                    completedTests[tName] = 0
                                    hasProgressOnTests = true
                                else
                                    Log.D(err.what)
                                    hasFailedTests = true
                                end
                            end
                        end
                        if hasProgressOnTests == false then
                            error(GenerationError("Stuck on tests generation"))
                        end
                    end
                end, ErrorHandler)
                if err == nil then
                    table.insert(sn, sName)
                    hasProgressOnSequences = true
                    completedSequences[sName] = 0
                else
                    if type(err) == "string" or err.code ~=
                        GenerationError().code then
                        error(err)
                    else
                        Log.D(err.what)
                        hasFailedSequences = true
                    end
                end
            end
        end
        if hasProgressOnSequences == false then
            error(GenerationError("Stuck on sequence generation"))
        end
    end
    Context.orchestrator.scope = nil
    local edges = { first = { sequence = nil, tests = {} }, last = { sequence = nil, tests = {} } }

    for _, requirement in ipairs(Context.orchestrator.order_requirements) do
        if not Orchestrator.HasScope(requirement.scope) then error(InvalidRequirement()) end
        if requirement.kind == OrderRequirement.Kind.first then
            Sort.AddEdgeRequirement(edges.first, requirement)
        elseif requirement.kind == OrderRequirement.Kind.last then
            Sort.AddEdgeRequirement(edges.last, requirement)
        elseif requirement.kind == OrderRequirement.Kind.after then
            if requirement.reference == nil then error(InvalidRequirement("no reference", requirement)) end
            if requirement.reference.sequence == nil then error(InvalidRequirement("no reference sequence", requirement)) end
            if requirement.scope.sequence == nil then error(InvalidRequirement("no scope sequence", requirement)) end
            if not Orchestrator.HasScope(requirement.reference) then error(InvalidRequirement(
                "reference scope not found", requirement)) end
            if requirement.scope.sequence == requirement.reference.sequence then
                if requirement.scope.test == nil then error(InvalidRequirement("scope test not found", requirement)) end
                if requirement.reference.test == nil then error(InvalidRequirement("reference test no found", requirement)) end
                td[requirement.scope.sequence][requirement.scope.test][requirement.reference.test] = 0
            else
                sd[requirement.scope.sequence][requirement.reference.sequence] = 0
            end
        else
            error(InvalidRequirement("generic error", requirement))
        end
    end

    for _, requirement in pairs(Context.orchestrator.sync_requirements) do
        if requirement.scope.sequence == nil then error(InvalidRequirement("no scope sequence", requirement)) end
        if not Orchestrator.HasScope(requirement.scope) then error(InvalidRequirement("scope not found", requirement)) end
        if requirement.scope.test ~= nil then
            ss[requirement.scope.sequence] = 0
            ts[requirement.scope.sequence][requirement.scope.test] = 0
        else
            ss[requirement.scope.sequence] = 0
        end
    end

    local ordered = {}
    ordered.sequences = Sort.SortScopes(sn, edges.first.sequence,
        edges.last.sequence, sd)
    ordered.tests = {}
    for sequence, tests in pairs(tn) do
        ordered.tests[sequence] = Sort.SortScopes(tests,
            edges.first.tests[sequence],
            edges.last.tests[sequence],
            td[sequence])
    end

    local sectionized = {}
    sectionized.sequences = Sort.Sectionize(ordered.sequences, ss)
    sectionized.tests = {}
    for sequence, tests in pairs(ordered.tests) do
        sectionized.tests[sequence] = Sort.Sectionize(tests, ts[sequence])
    end

    Context.orchestrator.solution = Sort.CombineSectionized(
        sectionized.sequences, sectionized.tests)
    return Context.orchestrator.solution
end

function Orchestrator.Validate()
    Log.D("Validation")
    for sIndex, section in ipairs(Context.orchestrator.solution) do
        Context.orchestrator.section = section
        for _, sequenceStage in ipairs(section) do
            for _, sequence in ipairs(sequenceStage) do
                Orchestrator.RunSequence(sIndex, Scope:New(sequence.name))
            end
        end
    end
end

function Orchestrator.ExecuteSection(index)
    Log.D("Section execution : " .. tostring(index))
    local section = Context.orchestrator.solution[index]
    local results = { time = {} }
    if index == 1 then
        Context.orchestrator.section = {}
        Context.orchestrator.section = {}
        Context.orchestrator.section.results = {}
        Context.info.time.start = os.clock()
        results.time.start = os.clock()
    end
    for _, sequenceStage in ipairs(section) do
        for _, sequence in ipairs(sequenceStage) do
            Orchestrator.RunSequence(index, Scope:New(sequence.name))
        end
    end
    results.time.stop = os.clock()
    Context.orchestrator.section.results[index] = results
end

function Orchestrator.CompileExecutionResults(outputDir)
    local report = {}
    report.info = {
        version = {
            frasy = Context.info.version.frasy,
            orchestrator = Context.info.version.orchestrator,
            scripts = Context.info.version.scripts
        },
        title = Context.info.title,
        operator = Context.info.operator,
        serial = Context.info.serial,
        uut = Context.info.uut,
        date = os.date(),
        pass = true,
        time = {
            start = Context.info.time.start,
            stop = os.clock(),
            elapsed = os.clock() - Context.info.time.start,
            process = 0
        }
    }
    report.ib = {}
    for k, ib in pairs(Context.map.ibs) do
        report.ib[k] = {
            kind = ib.ib.kind,
            nodeId = ib.ib.nodeId,
            eds = ib.ib.eds,
            software = ib.ib:SoftwareVersion(),
            hardware = ib.ib:HardwareVersion(),
            serial = tostring(ib.ib:Serial()),
        }
    end

    report.sequences = {}
    for sName, sequence in pairs(Context.orchestrator.sequences) do
        if not sequence.result.pass then
            report.info.pass = false
        end
        report.sequences[sName] = sequence.result
        report.sequences[sName].tests = {}
        for tName, test in pairs(sequence.tests) do
            report.sequences[sName].tests[tName] = test.result
            report.sequences[sName].tests[tName].expectations = {}
            for _, expectation in pairs(test.expectations) do
                table.insert(report.sequences[sName].tests[tName].expectations,
                    expectation)
            end
        end
        report.sequences[sName].time = { start = 0, stop = 0, elapsed = 0, process = 0 }
        if sequence.time ~= nil then
            report.sequences[sName].time.start = sequence.time[1].start
            for _, time in ipairs(sequence.time) do
                report.sequences[sName].time.stop = time.stop
                report.sequences[sName].time.process = report.sequences[sName].time.process + time.stop - time.start
            end
            report.sequences[sName].time.elapsed = report.sequences[sName].time.stop -
                report.sequences[sName].time.start
            report.info.time.process = report.info.time.process +
                report.sequences[sName].time.process
        end
    end
    Context.map.onReport(report) -- report passed as pointer, careful
    SaveAsJson(report, string.format("%s/%s.json", outputDir, Context.info.uut))
end

function Orchestrator.SaveSolution(path)
    SaveAsJson(Context.orchestrator.solution, path)
end

function Orchestrator.LoadSolution(path)
    local file = io.open(path, "r")
    Context.orchestrator.solution = Json.decode(file:read("*all"))
end

function Orchestrator.CreateSequence(name, func, source, line)
    if Orchestrator.IsInSequence() then
        error(NestedScope())
    end
    if Orchestrator.HasSequence(Scope:New(name)) then
        error(AlreadyDefined())
    end
    Context.orchestrator.sequences[name] = Sequence:New(func, source, line)
    Context.orchestrator.values[name] = {}
end

function Orchestrator.CreateTest(name, func, source, line)
    if not Orchestrator.IsInSequence() then
        error(BadScope())
    end
    if Orchestrator.IsInTest() then
        error(NestedScope())
    end
    Context.orchestrator.sequences[Context.orchestrator.scope.sequence].tests[name] = Test:New(func, name, source, line)
    Context.orchestrator.values[Context.orchestrator.scope.sequence][name] = {}
end

function Orchestrator.GetSequenceScopeRequirement(name)
    if not Orchestrator.IsInSequence() then
        error(BadScope())
    end
    if name == nil then
        name = Context.orchestrator.scope.sequence
    end
    return ScopeRequirement:New(Orchestrator, Scope:New(name, nil))
end

function Orchestrator.GetTestScopeRequirement(name)
    if not Orchestrator.IsInSequence() then
        error(BadScope())
    end
    if name == nil then
        if not Orchestrator.IsInTest() then
            error(BadScope())
        end
        name = Context.orchestrator.scope.test
    end
    return ScopeRequirement:New(Orchestrator, Scope:New(
        Context.orchestrator.scope.sequence, name))
end

function Orchestrator.GetSyncRequirement()
    if not Orchestrator.IsInSequence() then
        error(BadScope())
    end
    return SyncRequirement:New(Context.orchestrator.scope)
end

function Orchestrator.IsInSequence()
    return Context.orchestrator.scope ~= nil
end

function Orchestrator.IsInTest()
    return
        Context.orchestrator.scope ~= nil and Context.orchestrator.scope.test ~=
        nil
end

function Orchestrator.HasSequence(scope)
    return Context.orchestrator.sequences[scope.sequence] ~= nil
end

function Orchestrator.HasTest(scope)
    return Orchestrator.HasSequence(scope) and
        Context.orchestrator.sequences[scope.sequence].tests[scope.test] ~=
        nil
end

function Orchestrator.HasScope(scope)
    return scope.test == nil and Orchestrator.HasSequence(scope) or
        Orchestrator.HasTest(scope)
end

function Orchestrator.HasValue(scope, name)
    if not Orchestrator.HasScope(scope) then
        error(BadScope())
    end
    return Context.orchestrator.values[scope.sequence][scope.test][name] ~= nil
end

function Orchestrator.SetValue(scope, name, value)
    if Orchestrator.HasValue(scope, name) and Context.info.stage ~=
        Stage.generation then
        error(AlreadyDefined())
    end
    Context.orchestrator.values[scope.sequence][scope.test][name] = value
end

function Orchestrator.GetValue(scope, name)
    if not Orchestrator.HasValue(scope, name) then
        error(NotFound())
    end
    return Context.orchestrator.values[scope.sequence][scope.test][name]
end

function Orchestrator.AddOrderRequirement(requirement)
    if not Orchestrator.IsInSequence() then
        error(BadScope())
    end
    table.insert(Context.orchestrator.order_requirements, requirement)
end

function Orchestrator.AddSyncRequirement(requirement)
    if not Orchestrator.IsInSequence() then
        error(BadScope())
    end
    table.insert(Context.orchestrator.sync_requirements, requirement)
end

function Orchestrator.AddExpectationResult(result)
    if not Orchestrator.IsInTest() then
        error(BadScope())
    end
    local scope = Context.orchestrator.scope
    table.insert(
        Context.orchestrator.sequences[scope.sequence].tests[scope.test]
        .expectations, result)
end

function Orchestrator.GetScope()
    return Context.orchestrator.scope
end

function Orchestrator.HasPassed(scope)
    if not Orchestrator.HasScope(scope) then
        error(NotFound(scope:ToString()))
    end
    local s = Context.orchestrator.sequences[scope.sequence]
    if scope.test ~= nil then
        s = s.tests[scope.test]
    end
    return (not s.result.skipped) and s.result.pass
end

function Orchestrator.HasBeenSkipped(scope)
    if not Orchestrator.HasScope(scope) then
        error(NotFound(scope:ToString()))
    end
    local s = Context.orchestrator.sequences[scope.sequence]
    if scope.test ~= nil then
        s = s.tests[scope.test]
    end
    return s.result.skipped
end

function Orchestrator.Enable(sequence, test)
    local scope = Scope:New(sequence, test)
    if not Orchestrator.HasScope(scope) then
        error(BadScope("S: %s, T: %s", tostring(sequence), tostring(test)))
    end
    local s = Context.orchestrator.enable_list[sequence]
    if s == nil then
        s = { enabled = true }
    end
    if test ~= nil then
        s[test] = true
    end
    Context.orchestrator.enable_list[sequence] = s
end

function Orchestrator.Disable(sequence, test)
    local scope = Scope:New(sequence, test)
    if not Orchestrator.HasScope(scope) then
        error(BadScope("S: %s, T: %s", tostring(sequence), tostring(test)))
    end
    local s = Context.orchestrator.enable_list[sequence]
    if s == nil then
        s = { enabled = true }
    end
    if test == nil then
        s = { enabled = false }
    else
        s[test] = false
    end
    Context.orchestrator.enable_list[sequence] = s
end
