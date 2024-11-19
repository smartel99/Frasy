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

function Sequence(name, func)
    if func == nil then
        return Orchestrator.GetSequenceScopeRequirement(name)
    else
        local ar = debug.getinfo(2, "Sl")
        Orchestrator.CreateSequence(name, func, ar.source, ar.currentline)
    end
end

---Defines a test to be ran, or fetches informations about a test.
---
---Must be called from within a Sequence.
---@param name string Name of the test. This will appear in the log.
---@param func function Body of the test.
---@return table?
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
