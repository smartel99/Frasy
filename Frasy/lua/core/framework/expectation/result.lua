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

local ExpectationResult   = {
    value    = nil,
    note     = nil,
    pass     = false,
    inverted = false,
}
ExpectationResult.__index = ExpectationResult

function ExpectationResult:New(value, note)
    return setmetatable({
                            value    = value,
                            note     = note,
                            pass     = false,
                            inverted = false,
                        }, ExpectationResult)
end

return ExpectationResult