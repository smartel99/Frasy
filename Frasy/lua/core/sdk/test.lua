--- @file    framework.lua
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

local Expectation = require("lua/core/framework/expectation/module")
local RuntimeRequirement = require("lua/core/framework/runtime_requirement")


--- Sequence setter and Sequence scope getter
--- Sequence Creation
--- In order to create a sequence, you must provide both a name and function
--- The orchestrator will register the sequence and run it accordingly
--- Sequence name must be unique
--- Sequence function must contains at least one test
--- ```lua
--- Sequence("MySequence", function()
---     ... -- my tests
--- end)
--- ```
--- Sequence Scope Getter
--- When running a test, you can fetch a scope filtered on said sequence
--- To do so, you can provide the name of the sequence.
--- If not provided, will return the current sequence
--- ```lua
--- Sequence("MyFirstSequence", function()
---     Requires(Sequence():ToBeFirst()) -- Get current sequence scope
---     ... -- sequence tests
--- end)
--- Sequence("MySecondSequence", function()
---     Requires(Sequence("MyFirstSequence"):ToPass()) -- Get MyFirstSequence scope
---     ... -- sequence tests
--- end)
--- ```
--- @param name string? Name of the sequence to access
--- @param func function? Body of the sequence
--- @return Scope? scope when using function as getter
function Sequence(name, func)
    if func == nil then
        return Orchestrator.GetSequenceScopeRequirement(name)
    else
        local ar = debug.getinfo(2, "Sl")
        Orchestrator.CreateSequence(name, func, ar.source, ar.currentline)
    end
end

--- Test setter and Test scope getter  
--- Test Creation
--- In order to create a test, you must provide both a name and function
--- The orchestrator will register the test and run it accordingly inside its sequence
--- Test name must be unique among same sequence
--- Test creation must be done inside a sequence
--- ```lua
--- Sequence("MySequence", function()
---     Test("MyTest", function()
---     end)
--- end)
--- ```
--- Test Scope Getter
--- When running a test, you can fetch a scope filtered by the test name.
--- When provided, will return the Scope for a test with said name among the same sequence.
--- When not provided, will return current Scope.
--- You can link this call with Sequence() in order to get the scope of a test among a different sequence
--- ```lua
--- Sequence("Sequence1", function()
---     Test("MySequence1Test", function()
---     end)
--- end)
--- Sequence("Sequence2", function()
---     Test("MySequence2Test1", function()
---         Requires(Test():ToBeFirst()) -- Fetch current scope
---     end)
---     Test("MySequence2Test2", function()
---         Requires(Test("MySequence2Test1"):ToPass()) -- Fetch scope for a test on same sequence
---     end)
---     Test("MySequence2Test3", function()
---         Requires(Sequence("Sequence1")
---                     :Test("MySequence1Test")
---                     :ToPass()) -- Fetch scope for a test on a different sequence
---     end)
--- end)
--- ```
---@param name string? Name of the test. This will appear in the log
---@param func function? Body of the test
---@return Scope? test when using function as getter
function Test(name, func)
    if func == nil then
        return Orchestrator.GetTestScopeRequirement(name)
    else
        local ar = debug.getinfo(2, "Sl")
        Orchestrator.CreateTest(name, func, ar.source, ar.currentline)
    end
end

function Requires(requirement)
    if not requirement:IsMet() then
        error(UnmetRequirement(requirement.reason))
    end
end

function Sync()
    local requirement = Orchestrator.GetSyncRequirement()
    Orchestrator.AddSyncRequirement(requirement)
    return requirement
end

function RequirementSpecifier(func)
    return RuntimeRequirement:New(func)
end

---Expect
---@param value any
---@param note string
---@return Expectation
function Expect(value, note)
    return Expectation:New(value, note)
end

function Exclusive(value, func)
    __exclusive(value, func)
end
