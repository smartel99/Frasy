--- @file    equals.lua
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

local function Equals(t1, t2)
    if type(t1) ~= type(t2) then
        return false
    end
    if type(t1) == 'table' then
        -- Check both table have the same number of keys
        -- If not, then tables cannot be identical
        local t1_keys = 0
        local t2_keys = 0
        for k in pairs(t1) do
            t1_keys = t1_keys + 1
        end
        for k in pairs(t2) do
            t2_keys = t2_keys + 1
        end
        if t1_keys ~= t2_keys then
            return false
        end

        -- Check value of both table
        -- It also check that they share the same key since if they do not, one value would be nil
        for k, _ in pairs(t1) do
            if not Equals(t1[k], t2[k]) then
                return false
            end
        end
        return true
    else
        return t1 == t2
    end
end

return Equals
