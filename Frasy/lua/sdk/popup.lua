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
    text   = 0,
    input  = 1,
    button = 2,
    image  = 3,
}

function PopupBuilder:New(name)
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
        kind  = ElementKind.text,
        value = text,
    })
    return self
end

function PopupBuilder:Image(path, width, height)
    if(width == nil) then width = 0 end
    if(height == nil) then height = 0 end
    table.insert(self.elements, {
        kind = ElementKind.image,
        value = path,
        width = width,
        height = height
    })
    return self
end

function PopupBuilder:Input(text)
    if text == nil then text = "" end
    table.insert(self.elements, {
        kind  = ElementKind.input,
        value = text,
    })
    return self
end

function PopupBuilder:Button(text, action)
    table.insert(self.elements, {
        kind   = ElementKind.button,
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
    if (Context.info.stage == Stage.execution) then
        __popup.Consume(self)
    end
end

function PopupBuilder:Show()
    if (Context.info.stage == Stage.execution) then
        __popup.Show(self)
    end
end

function Popup(name)
    return PopupBuilder:New(name)
end