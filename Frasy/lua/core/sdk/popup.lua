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

local IsInteger = require("lua/core/utils/is_integer/is_integer")
local CheckField = require("lua/core/utils/check_field")
local function PrepareOptParameters(opt)
    if opt == nil then return {} end
    CheckField(opt, "opt", type(opt) == "table")
    return opt
end
local function PrepareImguiSize(value, name)
    if (value == nil) then return { 0.0, 0.0 } end
    CheckField(value, name .. " type", type(value) == "table")
    if value.width == nil then value.width = 0.0 end
    if value.height == nil then value.height = 0.0 end
    CheckField(value.width, name .. " width", type(value.width) == "number");
    CheckField(value.height, name .. " height", type(value.height) == "number");
    return { value.width * 1.0, value.height * 1.0 }
end

---@enum ElementKind
local ElementKind    = {
    text            = 0,
    textDynamic     = 1,
    input           = 2,
    button          = 3,
    image           = 4,
    beginHorizontal = 5,
    endHorizontal   = 6,
    beginVertical   = 7,
    endVertical     = 8,
    sameLine        = 9,
    spring          = 10,
}

--- @class PopupBuilder
--- @field name string
--- @field initialPosition number[]?
--- @field elements ElementKind[]
--- @field global boolean
local PopupBuilder   = { name = "", elements = {}, global = false }
PopupBuilder.__index = PopupBuilder

---@class ImGui_Size
---@field width number?
---@field height number?

---Creates a new popup.
---@param name string? Name to be displayed.
---@param initialPosition number[]?
---@return PopupBuilder
function PopupBuilder:New(name, initialPosition)
    if name == nil then name = "" end
    return setmetatable(
        {
            name            = name,
            elements        = {},
            initialPosition = initialPosition,
            global          = false,
        },
        PopupBuilder)
end

function PopupBuilder:Global()
    self.global = true
    return self
end

--- Show a label on popup
---@param text string
---@return PopupBuilder builder
function PopupBuilder:Text(text)
    CheckField(text, "text", type(text) == "string")
    table.insert(self.elements, {
        kind = ElementKind.text,
        text = text,
    })
    return self
end

--- Create a label with dynamic text
---@param routine function
---@return PopupBuilder builder
function PopupBuilder:TextDynamic(routine)
    CheckField(routine, "routine", type(routine) == "function")
    table.insert(self.elements, {
        kind = ElementKind.textDynamic,
        routine = routine,
    })
    return self
end

--- Show a TextInput on popup
--- @param title string
--- @return PopupBuilder builder
function PopupBuilder:Input(title)
    CheckField(title, "title", type(title) == "string")
    table.insert(self.elements, {
        kind = ElementKind.input,
        title = title,
    })
    return self
end

--- @class Popup_Button_OptParameters
--- @field size ImGui_Size?
--- @field consume boolean? tell if this button must consume popup on click

--- Show a button that can trigger an action
--- @param label string
--- @param action function must take no parameter and return no value
--- @param opt Popup_Button_OptParameters?
--- @return PopupBuilder builder
function PopupBuilder:Button(label, action, opt)
    CheckField(label, "label", type(label) == "string")
    CheckField(action, "action", type(action) == "function")
    opt = PrepareOptParameters(opt)
    opt.size = PrepareImguiSize(opt.size, "size")
    if opt.consume == nil then opt.consume = false end
    CheckField(opt.consume, "consume", type(opt.consume) == "boolean")
    table.insert(self.elements, {
        kind    = ElementKind.button,
        label   = label,
        action  = action,
        size    = opt.size,
        consume = opt.consume,
    })
    return self
end

--- Show an image
--- @param path string
--- @param size ImGui_Size?
function PopupBuilder:Image(path, size)
    CheckField(path, "path", type(path) == "string")
    size = PrepareImguiSize(size, "size")
    table.insert(self.elements, {
        kind = ElementKind.image,
        path = path,
        size = size
    })
    return self
end

--- @class ImGui_Layout_OptParameters
--- @field size ImGui_Size?
--- @field align number?

--- @param id integer
--- @param opt ImGui_Layout_OptParameters?
--- @return PopupBuilder
function PopupBuilder:BeginHorizontal(id, opt)
    CheckField(id, "id", IsInteger(id))
    opt = PrepareOptParameters(opt)
    opt.size = PrepareImguiSize(opt.size, "size")
    if opt.align == nil then opt.align = -1.0 end
    CheckField(opt.align, "align", type(opt.align) == "number")
    table.insert(self.elements, {
        kind = ElementKind.beginHorizontal,
        id = id,
        size = opt.size,
        align = opt.align,
    })
    return self
end

--- @return PopupBuilder
function PopupBuilder:EndHorizontal()
    table.insert(self.elements, { kind = ElementKind.endHorizontal })
    return self
end

--- @param id integer
--- @param opt ImGui_Layout_OptParameters?
--- @return PopupBuilder
function PopupBuilder:BeginVertical(id, opt)
    CheckField(id, "id", IsInteger(id))
    opt = PrepareOptParameters(opt)
    opt.size = PrepareImguiSize(opt.size, "size")
    if opt.align == nil then opt.align = -1.0 end
    CheckField(opt.align, "align", type(opt.align) == "number")
    table.insert(self.elements, {
        kind = ElementKind.beginVertical,
        id = id,
        size = opt.size,
        align = opt.align,
    })
    return self
end

--- @return PopupBuilder
function PopupBuilder:EndVertical()
    table.insert(self.elements, { kind = ElementKind.endVertical })
    return self
end

--- @class ImGui_SameLine_OptParameters
--- @field offsetFromStartX number?
--- @field spacing number?

--- Request next Element to be on same line as previous
--- @param opt ImGui_SameLine_OptParameters?
--- @return PopupBuilder builder
function PopupBuilder:SameLine(opt)
    if opt == nil then opt = {} end
    CheckField(opt, "opt", type(opt) == "table")
    if opt.offsetFromStartX == nil then opt.offsetFromStartX = 0.0 end
    if opt.spacing == nil then opt.spacing = -1.0 end
    CheckField(opt.offsetFromStartX, "offsetFromStartX", type(opt.offsetFromStartX) == "number");
    CheckField(opt.spacing, "spacing", type(opt.spacing) == "number");
    table.insert(self.elements, {
        kind = ElementKind.sameLine,
        offsetFromStartX = opt.offsetFromStartX,
        spacing = opt.spacing,
    })
    return self
end

--- @class ImGui_Spring_OptParameters
--- @field weight number?
--- @field spacing number?

--- @param opt ImGui_Spring_OptParameters?
function PopupBuilder:Spring(opt)
    opt = PrepareOptParameters(opt)
    if opt.weight == nil then opt.weight = 1 end
    if opt.spacing == nil then opt.spacing = -1 end
    CheckField(opt.weight, "weight", type(opt.weight) == "number")
    CheckField(opt.spacing, "spacing", type(opt.spacing) == "number")
    table.insert(self.elements, {
        kind = ElementKind.spring,
        weight = opt.weight,
        spacing = opt.spacing,
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
---@param initialPosition number[]?
---@return PopupBuilder
function Popup(name, initialPosition)
    return PopupBuilder:New(name, initialPosition)
end
