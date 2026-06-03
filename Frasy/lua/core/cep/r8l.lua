--- @file    r8l.lua
--- @author  Paul Thomas
--- @date    2026-06-03
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

local Bitwise = require("lua/core/utils/bitwise")
local CheckField = require("lua/core/utils/check_field")
local Ib = require("lua/core/sdk/environment/ib")
local Is = require("lua/core/utils/is")

---@class R8L_Cache
---@field digitalOutput integer bitpacking of R8L_RelayStateOutput
---@field errorValueOutput integer bitpacking of R8L_RelayStateOutput

---@class R8L
---@field ib Ib?
---@field cache R8L_Cache
R8L = {}
R8L.__index = R8L

---@enum R8L_RelayStateEnum
R8L.RelayStateEnum = {
    open = 0,
    closed = 1
}

R8L.OPEN = R8L.RelayStateEnum.open
R8L.CLOSED = R8L.RelayStateEnum.closed

local function CheckIndex(index) CheckField(index, Is.IntegerIn, 0, 7) end

local function CheckRelayValue(value) CheckField(value, Is.Unsigned8) end

--- @class R8L_NewOptionalParameters
--- @field name string? default to "r8l"
--- @field nodeId integer? default to 4

--- Instantiates a R8L.
--- Only to be used during environment declaration.
--- @param opt R8L_NewOptionalParameters?
--- @return R8L
function R8L:New(opt)
    local ib = Ib:New()
    ib.kind = 04;
    if opt == nil then opt = {} end
    CheckField(opt, Is.Table)
    if opt.name == nil then opt.name = "r8l" end
    if opt.nodeId == nil then opt.nodeId = ib.kind end
    ib.name = opt.name
    ib.nodeId = opt.nodeId
    ib.eds = "lua/core/cep/eds/r8l_1.1.0.eds"
    return setmetatable({ ib = ib, cache = { digitalOutput = 0, errorValueOutput = 0 } }, R8L)
end

function R8L:Reset()
    self.ib:Reset()
end

function R8L:LoadCache()
    self:DigitalOutputs()
    self:ErrorValueOutputs()
end

--- DigitalOutputs Accessor
--- If value is provided, function will act as setter and return nothing.
---@param value integer? bitpacking of R8L_RelayStateEnum
---@return integer? value bitpacking of R8L_RelayStateEnum
function R8L:DigitalOutputs(value)
    local od = self.ib.od["Write Digital Output"]
    if value == nil then
        self.cache.digitalOutput = self.ib:Upload(od) --[[@as integer]]
        return self.cache.digitalOutput
    else
        CheckRelayValue(value)
        self.cache.digitalOutput = value
        self.ib:Download(od, value)
    end
end

--- DigitalOutputs Accessor
--- If value is provided, function will act as setter and return nothing.
---@param index integer
---@param value R8L_RelayStateEnum?
---@return R8L_RelayStateEnum?
function R8L:DigitalOutput(index, value)
    CheckIndex(index)
    if value == nil then
        return Bitwise.Extract(index, self:DigitalOutputs())
    else
        CheckRelayValue(value)
        self:DigitalOutputs(Bitwise.Inject(index, value, self.cache.digitalOutput))
    end
end

--- ErrorModeOutput Accessor
--- If value is provided, function will act as setter and return nothing
---@param value boolean?
---@return boolean?
function R8L:ErrorModeOutput(value)
    if value == nil then
        return self.ib:Upload(self.ib.od["Error Mode Output"]) --[[@as boolean]]
    else
        CheckField(value, Is.Boolean)
        self.ib:Download(self.ib.od["Error Mode Output"], value)
    end
end

--- ErrorValueOutputs Register Accessor
--- If value is provided, function will act as setter and return nothing
---@param value integer? bitpacking of R8L_RelayStateEnum
---@return integer? value bitpacking of R8L_RelayStateEnum
function R8L:ErrorValueOutputs(value)
    if value == nil then
        self.cache.errorValueOutput = self.ib:Upload(self.ib.od["Error Value Ouput"]) --[[@as integer]]
        return self.cache.errorValueOutput
    else
        CheckRelayValue(value)
        self.cache.errorValueOutput = value
        self.ib:Download(self.ib.od["Error Value Ouput"], value)
    end
end

--- ErrorValueOutput Bit Accessor
--- If value is provided, function will act as setter and return nothing
---@param index integer
---@param value R8L_RelayStateEnum?
---@return R8L_RelayStateEnum?
function R8L:ErrorValueOutput(index, value)
    CheckIndex(index)
    if value == nil then
        return Bitwise.Extract(index, self:ErrorValueOutputs())
    else
        CheckRelayValue(value)
        self:ErrorValueOutputs(Bitwise.Inject(index, value, self.cache.errorValueOutput))
    end
end

---@class R8L_Id
---@field ID_BRD_L number
---@field ID_BRD_R number

---Gets the IDs read by the board.
---@return {left: number, right: number}
function R8L:Id()
    local table = self.ib:Upload(self.ib.od["ID"]) --[[@as R8L_Id]]
    return { left = table.ID_BRD_L, right = table.ID_BRD_R }
end

function R8L:LogCodeDec(code)
    self.ib:Download(self.ib.od["LogCode"]["Dec"], code)
end

function R8L:LogCodeHex(code)
    self.ib:Download(self.ib.od["LogCode"]["Hex"], code)
end
