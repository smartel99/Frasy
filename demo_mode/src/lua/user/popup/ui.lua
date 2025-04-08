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
    Test("Basic Popup", function()
        Popup("My Basic Popup")
            :Text("My basic text")
            :Button("A button", function() Log.I("Button was pressed") end)
            :ConsumeButtonText("Pass") -- Default text is cancel
            :Show()
    end)

    Test("Routine Button", function()
        local start = os.clock()
        local popup = Popup("My Routine Button")
        popup:Text("This popup will disappear after 5 seconds")
        popup:Routine(function() if os.clock() - start > 5 then popup:Consume() end end)
        popup:ConsumeButtonText("Pass")
        popup:Show()
    end)

    Test("Advanced Popup", function()
        local state = false
        Popup("My Advanced Popup")
            :Button("Toggle", function() state = not state end)
            :SameLine({spacing = 25})
            :TextDynamic(function() return "State: " .. (state and "ON" or "OFF") end)
            :BeginHorizontal(1)
            :BeginVertical(2)
            :Button("Nice dragon", function() Log.I("I know right!") end)
            :Button("Bad dragon", function() Log.I("This will consume the popup") end, { consume = true })
            :EndVertical()
            :Image("assets/textures/icon.png", { width = 100, height = 100 })
            :EndHorizontal()
            :Show()
    end)
end)
