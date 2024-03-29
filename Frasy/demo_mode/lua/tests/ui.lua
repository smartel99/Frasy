--- @file    ui.lua
--- @author  Paul Thomas
--- @date    4/3/2023
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

Sequence("UI", function()
    Test("T1", function()
        local a1    = false
        local a2    = false
        local popup = Popup("My popup")
        popup:Text("Line 1"):Text("Line 2")
        popup:Input("Input 1"):Input("Input 2")
        popup:Button("Action 1", function(inputs)
            a1 = true
            Log.d("Click on Action 1")
        end)
        popup:Button("Action 2", function(inputs)
            a2 = true
            Log.d("Click on Action 2")
        end)
        popup:Text("Click on both button, and input Frasy and 42 in both inputs to close this popup")
        popup:Routine(function(inputs)
            if (a1 and a2 and inputs[1] == "Frasy" and inputs[2] == "42") then
                popup:Consume()
            end
        end)
        popup:Show()
    end)
end)