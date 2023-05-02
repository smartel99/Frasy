--- @file    print.lua
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

local utils = {}
function utils.print(t, lvl)
    if type(lvl) == "nil" then lvl = 0 end

    local str = ""
    for _ = 1, (lvl - 1) do str = str .. "    " end
    if lvl ~= 0 then str = str .. "|--- " end

    local fmt = string.format("%s%s: ", str, type(t))
    -- print(type(t))
    
    if type(t) == "table" then
        fmt = fmt .. "{\n"
        for k, v in pairs(t) do
            fmt = fmt .. str .. k .. ":" .. utils.print(v, lvl + 1) .. "\n"
            -- print(str .. k .. ":")
            -- utils.print(v, lvl + 1)
        end
        fmt = fmt .. str .. "}"
    else
        if type(t) == "string" then t = "\"" .. t .. "\"" end
        fmt = fmt .. str .. tostring(t) .. "\n"
        -- print(str .. tostring(t))
    end

    if lvl == 0 then Log.i(fmt) else return fmt end
end

return utils.print