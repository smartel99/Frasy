--- @file    scope_result.lua
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
local CheckField = require("lua/core/utils/check_field")
local Is = require("lua/core/utils/is")
local Maybe = require("lua/core/utils/maybe")

---@enum ScopeResultValue
local Value = {
    Pass = "pass",
    Fail = "fail",
    Skipped = "skipped",
    Disabled = "disabled",
    Incomplete = "incomplete",
}

local values = {}
for _, v in pairs(Value) do table.insert(values, v) end

---@class ScopeResult
---@field time {start: number, stop: number, elapsed: number, process: number}
---@field result ScopeResultValue
---@field reason string?
---@field pass boolean Use `result`. Only v1 reports carry it.
---@field skipped boolean Use `result`. Only v1 reports carry it.
---@field enabled boolean Use `result`. Only v1 reports carry it.
local ScopeResult = { Value = Value }
ScopeResult.__index = ScopeResult

--- A scope that never ran counts as failed.
---@return ScopeResult
function ScopeResult:New()
    local result = setmetatable({ time = { start = 0, stop = 0, elapsed = 0, process = 0 } }, ScopeResult)
    result:Set(Value.Fail)
    return result
end

---@param value ScopeResultValue
---@param reason string?
function ScopeResult:Set(value, reason)
    CheckField(value, Is.InArray, values)
    CheckField(reason, Maybe, Is.String)
    self.result = value
    self.reason = reason
    self.pass = value == Value.Pass
    self.skipped = value == Value.Skipped or value == Value.Disabled
    self.enabled = value ~= Value.Disabled
end

---@return boolean
function ScopeResult:HasPassed() return self.result == Value.Pass end

--- Disabled scopes count as skipped.
---@return boolean
function ScopeResult:IsSkipped() return self.result == Value.Skipped or self.result == Value.Disabled end

--- Combine children results by precedence: fail > incomplete > pass. Skipped children are neutral.
---@param children ScopeResultValue[]
---@return ScopeResultValue
function ScopeResult.Aggregate(children)
    local aggregate = Value.Pass
    for _, child in ipairs(children) do
        if child == Value.Fail then return Value.Fail end
        if child == Value.Disabled or child == Value.Incomplete then aggregate = Value.Incomplete end
    end
    return aggregate
end

--- Combine results into the board result, by precedence: incomplete > fail > pass.
--- Anything disabled makes the whole board a debug run, even when something that ran failed. Tests are read too:
--- a sequence holding a disabled test and a failing one reads `fail`, and a sequence cut short by an error in a later
--- section keeps the result of its earlier sections.
---@param sequences ScopeResultValue[]
---@param tests ScopeResultValue[] Results of every test of every sequence.
---@return ScopeResultValue
function ScopeResult.AggregateBoard(sequences, tests)
    for _, results in ipairs({ sequences, tests }) do
        for _, result in ipairs(results) do
            if result == Value.Disabled or result == Value.Incomplete then return Value.Incomplete end
        end
    end
    for _, result in ipairs(tests) do
        if result == Value.Fail then return Value.Fail end
    end
    return ScopeResult.Aggregate(sequences)
end

return ScopeResult
