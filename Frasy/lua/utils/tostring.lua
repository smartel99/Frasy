--- @file    tostring.lua
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
function utils.tostring(t, lvl)
    if type(lvl) == "nil" then lvl = 0 end

    local str = ""
    for _ = 1, (lvl - 1) do str = str .. "    " end
    if lvl ~= 0 then str = str .. "|--- " end

    if type(t) == "table" or type(t) == "userdata" then
        for k, v in pairs(t) do
            str = str .. k .. ":\r\n" .. utils.tostring(v, lvl + 1)
        end
    else
        if type(t) == "string" then t = "\"" .. t .. "\"" end
        str = str .. tostring(t) .. "\r\n"
    end

    return str
end
return utils.tostring