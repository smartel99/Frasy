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
local Expectation       = { mandatory = false, result = nil }
Expectation.__index     = Expectation

function Expectation:new(value)
    return setmetatable({ mandatory = false, result = ExpectationResult:new(value) }, Expectation)
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
    self.result.method   = "ToBeTrue"
    self.result.expected = true
    self.result.pass     = self.result.value == self.result.expected
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeFalse()
    self.result.method   = "ToBeFalse"
    self.result.expected = false
    self.result.pass     = self.result.value == self.result.expected
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeEqual(expected)
    self.result.method   = "ToBeEqual"
    self.result.expected = expected
    self.result.pass     = self.result.value == self.result.expected
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

-- TODO handle negatives
function Expectation:ToBeNear(expected, deviation)
    self.result.parameters.method    = "ToBeNear"
    self.result.parameters.expected  = expected
    self.result.parameters.deviation = deviation
    self.result.pass                 = expected - deviation <= self.result.value and self.result.value <= expected + deviation
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeInRange(min, max)
    self.result.method = "ToBeInRange"
    self.result.min    = min
    self.result.max    = max
    self.result.pass   = min <= self.result.value and self.result.value <= max
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

-- TODO handle negatives
function Expectation:ToBeInPercentage(expected, percentage)
    self.result.method     = "ToBeInPercentage"
    self.result.expected   = expected
    self.result.percentage = percentage
    self.result.deviation  = expected * percentage
    self.result.pass       = expected - self.result.deviation <= self.result.value and self.result.value <= expected + self.result.deviation
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ToBeType(expected)
    self.result.method   = "ToBeType"
    self.result.expected = expected
    self.result.type     = type(self.result.value)
    self.result.pass     = self.result.type == expected
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

function Expectation:ExportAs(name)
    Orchestrator.SetValue(Orchestrator.GetScope(), name, self.result.value)
end

return Expectation