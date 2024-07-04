local Ib = require("lua/core/sdk/environment/ib")
PIO = {ib = nil, cache = {}}

PIO.SupplyEnum = {
    p3v3 = 1,
    p5v = 2,
    p12v = 3,
    p24v = 4,
    pVariable1 = 5,
    pVariable2 = 6
}

local function isSupplyEnumOk(supply)
    return p3v3 <= supply and supply <= pVariable2
end

local function isVariableSupplyEnumOk(supply)
    return supply == pVariable1 or supply == pVariable2
end

local function isCurrentLimitOk(value)
    return value ~= nil and type(value) == "number" and value // 1 == value and
               0 <= value and value <= 1100
end

local function isDesiredVoltageOk(value)
    return
        value ~= nil and type(value) == "number" and 1.08 <= value and value <=
            20.02
end

local function isGpioValueOk(value)
    return value ~= nil and type(value) == "number" and value // 1 == value and
               0 <= value and value <= 0xfff
end

function PIO:new(name, nodeId)
    local ib = Ib:new()
    ib.kind = 03;
    ib.name = name and name ~= nil or "pio"
    ib.nodeId = nodeId and nodeId ~= nil or 03
    ib.eds = "lua/core/cep/eds/pio_1.0.0.eds"
    return setmetatable({ib = ib, cache = {}})
end

local function supplyEnumToOdName(supply)
    local names = {
        "Supply 3V3", "Supply 5V", "Supply 12V", "Supply 24V",
        "Variable Supply 1", "Variable Supply 2"
    }
    if isSupplyEnumOk(supply) then
        return names[supply]
    else
        error("Invalid supply: " .. tostring(value))
    end
end

function PIO:currentLimit(supply, value)
    if not isSupplyEnumOk(supply) then
        error("Invalid supply: " .. tostring(value))
    end
    local odName = supplyEnumToOdName(supply)
    if value == nil then
        return self.id.Upload(self.id.od[odName]["Current Limit"])
    elseif isCurrentLimitOk(value) then
        self.id.Download(self.id.od[odName]["Current Limit"], value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function PIO:current(supply)
    if not isSupplyEnumOk(supply) then
        error("Invalid supply: " .. tostring(value))
    end
    local odName = supplyEnumToOdName(supply)
    return self.id.Upload(self.id.od[odName]["Current"])
end

function PIO:desiredVoltage(supply, value)
    if not isVariableSupplyEnumOk(supply) then
        error("Invalid supply: " .. tostring(value))
    end
    local odName = supplyEnumToOdName(supply)
    if value == nil then
        return self.id.Upload(self.id.od[odName]["Desired Voltage"])
    elseif isDesiredVoltageOk(value) then
        self.id.Download(self.id.od[odName]["Desired Voltage"], value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function PIO:voltage(supply)
    if not isSupplyEnumOk(supply) then
        error("Invalid supply: " .. tostring(value))
    end
    local odName = supplyEnumToOdName(supply)
    return self.id.Upload(self.id.od[odName]["Voltage"])
end

function PIO:ouputEnable(supply, value)
    if not isSupplyEnumOk(supply) then
        error("Invalid supply: " .. tostring(value))
    end
    local odName = supplyEnumToOdName(supply)
    if value == nil then
        return self.id.Upload(self.id.od[odName]["Output Enable"])
    elseif type(value) == "boolean" then
        self.id.Download(self.id.od[odName]["Output Enable"], value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function PIO:gpioIn() return self.id.Upload(self.id.od["GPIO"]["Input Port"]) end

function PIO:gpioOut(value)
    if value == nil then
        return self.id.Upload(self.id.od["GPIO"]["Output Port"])
    elseif isGpioValueOk(value) then
        self.id.Download(self.id.od["GPIO"]["Output Port"], value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function PIO:gpioPolarity(value)
    if value == nil then
        return self.id.Upload(self.id.od["GPIO"]["Polarity Inversion"])
    elseif isGpioValueOk(value) then
        self.id.Download(self.id.od["GPIO"]["Polarity Inversion"], value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function PIO:gpioConfiguration(value)
    if value == nil then
        return self.id.Upload(self.id.od["GPIO"]["Configuration"])
    elseif isGpioValueOk(value) then
        self.id.Download(self.id.od["GPIO"]["Configuration"], value)
    else
        error("Invalid value: " .. tostring(value))
    end
end
