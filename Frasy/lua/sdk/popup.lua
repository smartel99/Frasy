--- @file    popup.lua
--- @author  Paul Thomas
--- @date    3/30/2023
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

local PopupBuilder   = { name = "", elements = {}, global = false }
PopupBuilder.__index = PopupBuilder

local ElementKind    = {
    Text   = 0,
    Input  = 1,
    Button = 2,
}

function PopupBuilder.new(name)
    if name == nil then name = "" end
    return setmetatable(
            {
                name     = name,
                elements = {},
                global   = false,
            },
            PopupBuilder)
end

function PopupBuilder:Global()
    self.global = true
    return self
end

function PopupBuilder:Text(text)
    table.insert(self.elements, {
        kind  = ElementKind.Text,
        value = text,
    })
    return self
end

function PopupBuilder:Input(text)
    if text == nil then text = "" end
    table.insert(self.elements, {
        kind  = ElementKind.Input,
        value = text,
    })
    return self
end

function PopupBuilder:Button(text, action)
    table.insert(self.elements, {
        kind   = ElementKind.Button,
        value  = text,
        action = action,
    })
    return self
end

function PopupBuilder:Routine(routine)
    self.routine = routine
    return self
end

function PopupBuilder:Consume()
    if (Context.info.stage == Stage.Execution) then
        __popup.consume(self)
    end
end

function PopupBuilder:Show()
    if (Context.info.stage == Stage.Execution) then
        __popup.show(self)
    end
end

function Popup(name)
    return PopupBuilder.new(name)
end