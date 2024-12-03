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
local ExpectationResult = require("lua/core/framework/expectation/result")
---@class Expectation
---@field New function
---@field Mandatory function
---@field result ExpectationResult?
---
local Expectation = { mandatory = false, result = nil }
Expectation.__index = Expectation

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
    self.result.method = "ToBeTrue"
    self.result.expected = true
    self.result.pass = self.result.value == self.result.expected
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeFalse()
    self.result.method = "ToBeFalse"
    self.result.expected = false
    self.result.pass = self.result.value == self.result.expected
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeEqual(expected)
    self.result.method = "ToBeEqual"
    self.result.expected = expected
    self.result.pass = self.result.value == self.result.expected
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeNear(expected, deviation)
    self.result.method = "ToBeNear"
    self.result.expected = expected
    self.result.deviation = math.abs(deviation)
    self.result.min = expected - deviation
    self.result.max = expected + deviation
    self.result.pass = self.result.min <= self.result.value and
        self.result.value <= self.result.max
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeInRange(min, max)
    self.result.method = "ToBeInRange"
    self.result.min = min
    self.result.max = max
    self.result.pass = min <= self.result.value and self.result.value <= max
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeInPercentage(expected, percentage)
    self.result.method = "ToBeInPercentage"
    self.result.expected = expected
    self.result.percentage = percentage
    self.result.deviation = math.abs(expected * percentage / 100)
    self.result.min = expected - self.result.deviation
    self.result.max = expected + self.result.deviation
    self.result.pass = self.result.min <= self.result.value and self.result.value <= self.result.max
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeGreater(min)
    self.result.method = "ToBeGreater"
    self.result.min = min
    self.result.pass = (self.result.value > min)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)

    return self
end

function Expectation:ToBeGreaterOrEqual(min)
    self.result.method = "ToBeGreaterOrEqual"
    self.result.min = min
    self.result.pass = (self.result.value >= min)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)

    return self
end

function Expectation:ToBeLesser(max)
    self.result.method = "ToBeLesser"
    self.result.max = max
    self.result.pass = (self.result.value < max)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)

    return self
end

function Expectation:ToBeLesserOrEqual(max)
    self.result.method = "ToBeLesserOrEqual"
    self.result.max = max
    self.result.pass = (self.result.value <= max)
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)

    return self
end

function Expectation:ToBeType(expected)
    self.result.method = "ToBeType"
    self.result.expected = expected
    self.result.type = type(self.result.value)
    self.result.pass = self.result.type == expected
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToMatch(pattern)
    self.result.method = "ToMatch"
    self.result.pattern = pattern
    self.result.pass = false

    for w in string.gmatch(self.result.value, pattern) do
        self.result.pass = true
    end

    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ExportAs(name)
    Orchestrator.SetValue(Orchestrator.GetScope(), name, self.result.value)
end

return Expectation
