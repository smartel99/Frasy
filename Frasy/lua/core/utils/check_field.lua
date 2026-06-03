--- @file    check_field.lua
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

---Check if a value match a predicate
---Will throw if not
---@param v any field to check
---@param f fun(v: any, ...): boolean predicate function that must at least take v as its first parameter
---@param ... any? predicate additional parameters, if any
---@return any v the value that was provided
return function(v, f, ...)
    local function getName()
        local caller = debug.getinfo(3, "Sl")
        local selfInfo = debug.getinfo(2, "n")
        local funcName = selfInfo.name


        local file = io.open(caller.source:sub(2), "r")
        if not file then return nil end

        local currentLine = 0
        local targetLine = ""
        for line in file:lines() do
            currentLine = currentLine + 1
            if currentLine == caller.currentline then
                targetLine = line
                break
            end
        end
        file:close()
        local varName = targetLine:match(funcName .. "%s*%(%s*(.-)%s*,")
        return varName or "not found"
    end

    if not f(v, ...) then
        error(string.format("CheckField failed. %s: %s", tostring(getName()), ToString(v)), 2)
    end

    return v
end
