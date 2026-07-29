--- @file    try_function.lua
--- @author  Paul Thomas
--- @date    2026-06-03
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

local Is = require("lua/core/utils/is")
local CheckField = require("lua/core/utils/check_field")

---@class TryFunctionOptParam
---@field maxTryCount integer? default to 3
---@field delay integer? delay between each tries in ms, default to 0
---@field raiseError boolean? will raise an error if reach max tries, default to false

---@param fun fun(integer?): boolean
---@param opt TryFunctionOptParam?
---@return boolean, any?
local function tryFunction(fun, opt)
    CheckField(fun, Is.Function)
    opt = opt or {}
    CheckField(opt, Is.Table)
    local maxTryCount = opt.maxTryCount or 3
    local delay = opt.delay or 10
    local raiseError = opt.raiseError or false
    CheckField(maxTryCount, Is.Unsigned)
    CheckField(delay, Is.Unsigned)
    CheckField(raiseError, Is.Boolean)
    for try = 1, maxTryCount do
        if try ~= 1 and delay ~= 0 then SleepFor(delay) end

        local result, o = fun(try)
        if result then return result, o end
    end
    if raiseError then error("Reached tries limit") end
    return false
end

return tryFunction
