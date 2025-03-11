--- @file    order_requirement.lua
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
local OrderRequirement = { scope = nil, reference = nil, kind = nil }
OrderRequirement.__index = OrderRequirement
function OrderRequirement.__tostring(v)
    local function kindAsStr()
        if v.kind == 1 then
            return "First"
        elseif v.kind == 2 then
            return "Last"
        elseif v.kind == 3 then
            return "After"
        else
            return "Invalid (" .. v.kind .. ")"
        end
    end
    local str = "Kind: " .. kindAsStr() ..
        "\n\rRequested Scope:"
    if v.reference == nil or type(v.reference) ~= "table" then
        str = str .. "<Invalid>"
    else
        if v.reference.sequence ~= nil then str = str .. "\n\r\tSequence: " .. v.reference.sequence end
        if v.reference.test ~= nil then str = str .. "\n\r\tTest: " .. v.reference.test end
    end
    str = str .. "\n\rCurrent Scope:"
    if v.scope == nil or type(v.scope) ~= "table" then
        str = str .. "<Invalid>"
    else
        if v.scope.sequence ~= nil then str = str .. "\n\r\tSequence: " .. v.scope.sequence end
        if v.scope.test ~= nil then str = str .. "\n\r\tTest: " .. v.scope.test end
    end

    return str
end

OrderRequirement.Kind = { first = 1, last = 2, after = 3 }

function OrderRequirement:New(parent, target, kind)
    return setmetatable({ scope = parent, reference = target, kind = kind },
        OrderRequirement)
end

return OrderRequirement
