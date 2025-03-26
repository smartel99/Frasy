--- @file    exclusive.lua
--- @author  Paul Thomas
--- @date    4/4/2023
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

Sequence("Exclusive", function()
    Test("T1", function()
        -- Unique ID to define exclusive scope, must be an integer
        local id = 1

        -- function to run in exclusive scope
        local function routine()
            Log.I("UUT " .. tostring(Context.info.uut) .. " is now in exclusive region")
            SleepFor(1000)
        end

        -- Call function routine with scope id 1
        -- Only 1 uut can run in the same scope at a time
        -- Use different scope id if you need to run multiple UUT at the same time
        Exclusive(id, routine)
    end)
end)
