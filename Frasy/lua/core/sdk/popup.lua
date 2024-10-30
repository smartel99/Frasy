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

---@class PopupBuilder
---@field name string Name of the popup, appears in the ImGui window.
---@field elements table Elements of the popup.
---@field global boolean
---@field New function
---@field Global function
---@field Show function

local PopupBuilder   = { name = "", elements = {}, global = false }
PopupBuilder.__index = PopupBuilder

---@enum ElementKind
local ElementKind    = {
    text     = 0,
    input    = 1,
    button   = 2,
    image    = 3,
    sameLine = 4,
}

---Creates a new popup.
---@param name string Name to be displayed.
---@return PopupBuilder
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
        kind = ElementKind.text,
        text = text,
    })
    return self
end

function PopupBuilder:Image(path, width, height)
    if (width == nil) then width = 0 end
    if (height == nil) then height = 0 end
    table.insert(self.elements, {
        kind = ElementKind.image,
        path = path,
        width = width,
        height = height
    })
    return self
end

function PopupBuilder:Input(text)
    if text == nil then text = "" end
    table.insert(self.elements, {
        kind = ElementKind.input,
        text = text,
    })
    return self
end

function PopupBuilder:Button(text, action)
    table.insert(self.elements, {
        kind   = ElementKind.button,
        text   = text,
        action = action,
    })
    return self
end

function PopupBuilder:SameLine(width)
    if width == nil then width = 0 end
    table.insert(self.elements, {
        kind = ElementKind.sameLine,
        width = width,
    })
    return self
end

function PopupBuilder:Routine(routine)
    self.routine = routine
    return self
end

function PopupBuilder:ConsumeButtonText(text)
    self.consumeButtonText = text
    return self
end

function PopupBuilder:Consume()
    if (Context.info.stage == Stage.execution) then
        __popup.Consume(self)
    end
end

---Displays the popup.
function PopupBuilder:Show()
    if (Context.info.stage == Stage.execution) then
        __popup.Show(self)
    end
end

---Creates a new popup
---@param name string? Name of the popup.
---@return PopupBuilder
function Popup(name)
    return PopupBuilder:New(name)
end
