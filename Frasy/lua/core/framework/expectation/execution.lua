--- @file    execution.lua
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
require("lua/core/framework/expectation/common")
local ExpectationResult = require("lua/core/framework/expectation/result")
---@class Expectation
---@field New function
---@field Mandatory function
---@field result ExpectationResult?
---
local Expectation = { mandatory = false, result = nil }
Expectation.__index = Expectation

local function resultToSelfTable(result, self)
    for k, v in pairs(result) do
        self.result[k] = v
    end
end

---@param value any
---@param name string
---@param extra any?
function Expectation:New(value, name, extra)
    return setmetatable({
        mandatory = false,
        result = ExpectationResult:New(value, name, extra)
    }, Expectation)
end

local function enforce(expectation)
    if not expectation.mandatory then return end
    if expectation.result.pass == expectation.result.inverted then
        error(UnmetExpectation())
    end
end

function Expectation:Mandatory()
    self.mandatory = true
    return self
end

function Expectation:Not()
    self.result.inverted = not self.result.inverted
    return self
end

function Expectation:ToBeTrue()
    local result = ExpectToBeTrue(self.result.value)
    resultToSelfTable(result, self)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeFalse()
    local result = ExpectToBeFalse(self.result.value)
    resultToSelfTable(result, self)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeEqual(expected)
    local result = ExpectToBeEqual(self.result.value, expected)
    resultToSelfTable(result, self)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeNear(expected, deviation)
    local result = ExpectToBeNear(self.result.value, expected, deviation)
    resultToSelfTable(result, self)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeInRange(min, max)
    local result = ExpectToBeInRange(self.result.value, min, max)
    resultToSelfTable(result, self)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeInPercentage(expected, percentage)
    local result = ExpectToBeInPercentage(self.result.value, expected, percentage)
    resultToSelfTable(result, self)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeGreater(min)
    local result = ExpectToBeGreater(self.result.value, min)
    resultToSelfTable(result, self)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeGreaterOrEqual(min)
    local result = ExpectToBeGreaterOrEqual(self.result.value, min)
    resultToSelfTable(result, self)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeLesser(max)
    local result = ExpectToBeLesser(self.result.value, max)
    resultToSelfTable(result, self)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeLesserOrEqual(max)
    local result = ExpectToBeLesserOrEqual(self.result.value, max)
    resultToSelfTable(result, self)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeType(expected)
    local result = ExpectToBeType(self.result.value, expected)
    resultToSelfTable(result, self)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToMatch(pattern)
    local result = ExpectToBeMatch(self.result.value, pattern)
    resultToSelfTable(result, self)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ExportAs(name)
    Orchestrator.SetValue(Orchestrator.GetScope(), name, self.result.value)
end

function Expectation:Show()
    ShowExpectation(self.result)
end

return Expectation
