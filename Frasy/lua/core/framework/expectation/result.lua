--- @file    result.lua
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

---@class ExpectationResult
---@field value any
---@field name string
---@field note string? deprecated field, previously used as name/extra
---@field pass boolean tell if the value fulfilled requirement
---@field inverted boolean tell if the result should be interpreted with invert logic
---@field extra any? additional data that could be useful for developer
---@field method string
---@field expected any?
---@field min any?
---@field max any?
---@field deviation any?
---@field percentage any?
---@field type string?
local ExpectationResult   = {
    value    = nil,
    name     = "",
    note     = nil,
    pass     = false,
    inverted = false,
    extra    = nil,
}
ExpectationResult.__index = ExpectationResult


---@param value any
---@param name string
---@param extra any?
function ExpectationResult:New(value, name, extra)
    return setmetatable({
        value    = value,
        name     = name,
        note     = name,
        pass     = false,
        inverted = false,
        extra    = extra
    }, ExpectationResult)
end

return ExpectationResult
