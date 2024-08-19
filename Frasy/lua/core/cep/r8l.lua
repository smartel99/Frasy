local Ib = require("lua/core/sdk/environment/ib")
local Bitwise = require("lua/core/utils/bitwise")
local IsBoolean = require("lua/core/utils/is_boolean")
local IsIntegerIn = require("lua/core/utils/is_integer/is_integer_in")
local IsUnsigned8 = require("lua/core/utils/is_unsigned/is_unsigned_8")
local CheckField = require("lua/core/utils/check_field")

---@class R8L
---@field ib Ib?
R8L = {ib = nil, cache = {digitalOutput = 0, errorValueOutput = 0}}
R8L.__index = R8L

local function CheckIndex(index) CheckField(index, "index", IsIntegerIn(index, 0, 7)) end

local function CheckRelayValue(value) CheckField(value, "value", IsUnsigned8(value)) end

---Creates a new R8L
---@param name string?
---@param nodeId integer?
---@return R8L
function R8L:New(name, nodeId)
    local ib = Ib:New()
    ib.kind = 04;
    if name == nil then name = "r8l" end
    ib.name = name
    if nodeId == nil then nodeId = ib.kind end
    ib.nodeId = nodeId
    ib.eds = "lua/core/cep/eds/r8l_1.0.0.eds"
    return setmetatable({ib = ib, cache = {digitalOutput = 0, errorValueOutput = 0}}, R8L)
end

function R8L:Reset()
    self.ib:Reset()
end

function R8L:LoadCache()
    self:DigitalOutputs()
    self:ErrorValueOutputs()
end

function R8L:DigitalOutputs(value)
    local od = self.ib.od["Write Digital Output"]
    if value == nil then
        self.cache.digitalOutput = self.ib:Upload(od)
        return self.cache.digitalOutput
    else
        CheckRelayValue(value)
        self.cache.digitalOutput = value
        self.ib:Download(od, value)
    end
end

function R8L:DigitalOutput(index, value)
    CheckIndex(index)
    if value == nil then
        return Bitwise.Extract(index, self:DigitalOutputs())
    else
        CheckRelayValue(value)
        self:DigitalOutputs(Bitwise.Inject(index, value, self.cache.digitalOutput))
    end
end

function R8L:ErrorModeOutput(value)
    if value == nil then
        return self.ib:Upload(self.ib.od["Error Mode Output"])
    else
        CheckField(value, "value", IsBoolean(value))
        self.ib:Download(self.ib.od["Error Mode Output"], value)
    end
end

function R8L:ErrorValueOutputs(value)
    if value == nil then
        self.cache.errorValueOutput = self.ib:Upload(self.ib.od["Error Value Ouput"])
        return self.cache.errorValueOutput
    else
        CheckRelayValue(value)
        self.cache.errorValueOutput = value
        self.ib:Download(self.ib.od["Error Value Ouput"], value)
    end
end

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
    return {left = table.ID_BRD_L, right = table.ID_BRD_R}
end

return R8L

