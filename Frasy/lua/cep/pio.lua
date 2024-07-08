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

function PIO.IsSupplyEnumOk(supply)
    return p3v3 <= supply and supply <= pVariable2
end

function PIO.IsVariableSupplyEnumOk(supply)
    return supply == pVariable1 or supply == pVariable2
end

function PIO.IsCurrentLimitOk(value)
    return value ~= nil and type(value) == "number" and value // 1 == value and
               0 <= value and value <= 1100
end

function PIO.IsDesiredVoltageOk(value)
    return
        value ~= nil and type(value) == "number" and 1.08 <= value and value <=
            20.02
end

function PIO.IsGpioValueOk(value)
    return value ~= nil and type(value) == "number" and value // 1 == value and
               0 <= value and value <= 0xfff
end

function PIO.SupplyEnumToOdName(supply)
    local names = {
        "Supply 3V3", "Supply 5V", "Supply 12V", "Supply 24V",
        "Variable Supply 1", "Variable Supply 2"
    }
    if PIO.IsSupplyEnumOk(supply) then
        return names[supply]
    else
        error("Invalid supply: " .. tostring(value))
    end
end

function PIO:New(name, nodeId)
    local ib = Ib:New()
    ib.kind = 03;
    if name == nil then name = "pio" end
    ib.name = name
    if nodeId == nil then nodeId = ib.kind end
    ib.nodeId = nodeId
    ib.eds = "lua/core/cep/eds/pio_1.0.0.eds"
    return setmetatable({ib = ib, cache = {}}, PIO)
end

function PIO:CurrentLimit(supply, value)
    if not PIO.IsSupplyEnumOk(supply) then
        error("Invalid supply: " .. tostring(value))
    end
    local odName = PIO.SupplyEnumToOdName(supply)
    local od = self.id.od[odName]["Current Limit"]
    if value == nil then
        return self.id:Upload(od)
    elseif PIO.IsCurrentLimitOk(value) then
        self.id:Download(od, value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function PIO:Current(supply)
    if not PIO.IsSupplyEnumOk(supply) then
        error("Invalid supply: " .. tostring(value))
    end
    local odName = PIO.SupplyEnumToOdName(supply)
    return self.id:Upload(self.id.od[odName]["Current"])
end

function PIO:DesiredVoltage(supply, value)
    if not PIO.IsVariableSupplyEnumOk(supply) then
        error("Invalid supply: " .. tostring(value))
    end
    local odName = PIO.SupplyEnumToOdName(supply)
    local od = self.id.od[odName]["Desired Voltage"]
    if value == nil then
        return self.id:Upload(od)
    elseif PIO.IsDesiredVoltageOk(value) then
        self.id:Download(od, value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function PIO:Voltage(supply)
    if not PIO.IsSupplyEnumOk(supply) then
        error("Invalid supply: " .. tostring(value))
    end
    local odName = PIO.SupplyEnumToOdName(supply)
    return self.id:Upload(self.id.od[odName]["Voltage"])
end

function PIO:OuputEnable(supply, value)
    if not PIO.IsSupplyEnumOk(supply) then
        error("Invalid supply: " .. tostring(value))
    end
    local odName = PIO.SupplyEnumToOdName(supply)
    local od = self.id.od[odName]["Output Enable"]
    if value == nil then
        return self.id:Upload(od)
    elseif type(value) == "boolean" then
        self.id:Download(od, value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function PIO:GpioIn() return self.id:Upload(self.id.od["GPIO"]["Input Port"]) end

function PIO:GpioOut(value)
    local od = self.id.od["GPIO"]["Output Port"]
    if value == nil then
        return self.id:Upload(od)
    elseif PIO.IsGpioValueOk(value) then
        self.id:Download(od, value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function PIO:GpioPolarity(value)
    local od = self.id.od["GPIO"]["Polarity Inversion"]
    if value == nil then
        return self.id.Upload(od)
    elseif isGpioValueOk(value) then
        self.id.Download(od, value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function PIO:GpioConfiguration(value)
    local od = self.id.od["GPIO"]["Configuration"]
    if value == nil then
        return self.id:Upload(od)
    elseif PIO.IsGpioValueOk(value) then
        self.id:Download(od, value)
    else
        error("Invalid value: " .. tostring(value))
    end
end
