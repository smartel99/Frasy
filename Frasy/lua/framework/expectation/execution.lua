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

--- Defines an expectation from a test.
--- @class Expectation
--- @field private mandatory bool When true, a failure to meet this expectation results in the immediate termination of the test.
--- @field private result table Holds the value checked by the expectation.

local Expectation       = { mandatory = false, result = nil }
Expectation.__index     = Expectation

--- Constructor of expectation.
--- @param value any The value to be checked.
--- @param note string A note to be attached to the expectation
--- @return Expectation
function Expectation:new(value, note)
    return setmetatable({ mandatory = false, result = ExpectationResult:new(value, note) }, Expectation)
end

local function enforce(expectation)
    if not expectation.mandatory then return end
    if expectation.result.pass == expectation.result.inverted then
        error(UnmetExpectation())
    end
end

--- Marks the expectation as being mandatory for the continuation of the test.
--- @return Expectation
function Expectation:Mandatory()
    self.mandatory = true
    return self
end

--- Inverts the condition of the expectation.
--- @return Expectation
function Expectation:Not()
    self.result.inverted = not self.result.inverted
    return self
end

--- Indicates that the value must evaluate to true.
--- @return Expectation
function Expectation:ToBeTrue()
    self.result.method   = "ToBeTrue"
    self.result.expected = true
    self.result.pass     = self.result.value == self.result.expected
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

--- Indicates that the value must evaluate to false.
--- @return Expectation
function Expectation:ToBeFalse()
    self.result.method   = "ToBeFalse"
    self.result.expected = false
    self.result.pass     = self.result.value == self.result.expected
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

--- Indicates that the value must be equal to expected.
--- @param expected any The expected value. Must be of the same type, or comparable to the expectation's value.
--- @return Expectation
function Expectation:ToBeEqual(expected)
    self.result.method   = "ToBeEqual"
    self.result.expected = expected
    self.result.pass     = self.result.value == self.result.expected
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

-- TODO handle negatives
--- Indicates that the value must be within an allowed window of deviation from the expected value.
--- @param expected any The expected value. Must be of the same type, or comparable to the expectation's value.
--- @param deviation any The allowed deviation from the expected value. Must be of the same type asw the expected value.
--- @return Expectation
function Expectation:ToBeNear(expected, deviation)
    self.result.parameters.method    = "ToBeNear"
    self.result.parameters.expected  = expected
    self.result.parameters.deviation = deviation
    self.result.pass                 = expected - deviation <= self.result.value and self.result.value <= expected + deviation
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

--- Indicates that the value must be within a certain range.
--- @param min any The minimum allowed value.
--- @param max any The maximum allowed value.
--- @return Expectation
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
--- Indicates that the value must within an allowed window of deviation from the expected value.
--- @param expected number The expected value.
--- @param percentage number The allowed deviation, in percentage (100% being represented by 1.0).
--- @return Expectation
function Expectation:ToBeInPercentage(expected, percentage)
    self.result.method     = "ToBeInPercentage"
    self.result.expected   = expected
    self.result.percentage = percentage
    self.result.deviation  = expected * percentage
    self.result.pass       = (self.result.value >= (expected - self.result.deviation)) and 
                             (self.result.value <= (expected + self.result.deviation))
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

--- Indicates that the value must be of a certain type.
--- @param expected string The expected type.
--- @return Expectation
function Expectation:ToBeType(expected)
    self.result.method   = "ToBeType"
    self.result.expected = expected
    self.result.type     = type(self.result.value)
    self.result.pass     = self.result.type == expected
    Orchestrator.AddExpectationResult(self.result)
    enforce(self)
    return self
end

--- Indicates that the value must match a regular expression.
--- @param pattern string The pattern that must be met by the value.
--- @return Expectation
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

--- Makes the expectation's value available to other tests in the same sequence.
--- @param name string Name to be given to the exported value.
function Expectation:ExportAs(name)
    Orchestrator.SetValue(Orchestrator.GetScope(), name, self.result.value)
end

return Expectation