local Ib = require("lua/core/sdk/environment/ib")

R8L = {
    ib = nil,
    cache = {digitalOutput = 0, errorModeOutput = false, errorValueOutput = 0}
}
R8L.__index = R8L

local function isRelayValueOk(value)
    return value ~= nil and type(value) == "number" and value // 1 == value and
               0 <= value and value <= 255
end

function R8L:new(name, nodeId)
    local ib = Ib:new()
    ib.kind = 04;
    ib.name = name and name ~= nil or "r8l"
    ib.nodeId = nodeId and nodeId ~= nil or ib.kind
    ib.eds = "lua/core/cep/eds/r8l_1.0.0.eds"
    return setmetatable({ib = ib, od = {}}, R8L)
end

function R8L:test()
    Utils.print(self)
end

function R8L:digitalOutput(value)
    if value == nil then
        self.cache.digitalOutput = self.ib:Upload(self.ib.od["Write Digital Output"])
        return self.cache.digitalOutput
    elseif isRelayValueOk(value) then
        self.ib:Download(self.ib.od["Write Digital Output"], value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function R8L:errorModeOutput(value)
    if value == nil then
        self.cache.errorModeOutput = self.ib:Upload(
                                         self.ib.od["Error Mode Output"])
        return self.cache.errorModeOutput
    elseif type(value) == "boolean" then
        self.cache.errorModeOutput = value
        self.ib:Download(self.ib.od["Error Mode Output"], value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function R8L:errorValueOutput(value)
    if value == nil then
        self.cache.errorValueOutput = self.ib:Upload(
                                          self.ib.od["Error Value Output"])
        return self.cache.errorValueOutput
    elseif isRelayValueOk(value) then
        self.cache.errorValueOutput = value
        self.ib:Download(self.ib.od["Error Value Output"], value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function R8L:id()
    local table = self.ib:Upload(self.ib.od["ID"])
    return {left = table.ID_BRD_L, right = table.ID_BRD_R}
end

return R8L

