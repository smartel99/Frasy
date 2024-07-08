local Ib = require("lua/core/sdk/environment/ib")
local IsUnsigned8 = require("lua.core.utils.is_unsigned.is_unsigned_8")
local IsUnsigned = require("lua.core.utils.is_unsigned.is_unsigned")
local Bitwise = require("lua.core.utils.bitwise")
PIO = {
    ib = nil,
    cache = {
        gpio = {input = 0, output = 0, polarity = 0, configuration = 0x0FFF}
    }
}

PIO.SupplyEnum = {
    p3v3 = 1,
    p5v = 2,
    p12v = 3,
    p24v = 4,
    pVariable1 = 5,
    pVariable2 = 6
}

PIO.GpioIndexMax = 11

PIO.GpioPolarityEnum = {regular = 0, inverted = 1}

PIO.GpioConfigurationEnum = {output = 0, input = 1}

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

function PIO.CheckGpioIndex(index)
    assert(IsIntegerIn(channel, 0, PIO.GpioIndexMax),
           "Invalid gpio index: " .. tostring(index))
end

function PIO.NormalizeGpioValueOutput(value, configuration)
    for i = 0, PIO.GpioIndexMax do
        if Bitwise.Extract(index, configuration) ==
            PIO.GpioConfigurationEnum.input then
            value = Bitwise.Inject(index, Bitwise.Extract(index, value), value)
        else
            value = Bitwise.Inject(index, 0)
        end
    end
    return value
end

function PIO.MergeGpioValues(input, output, configuration)
    local merge = 0
    for i = 0, PIO.GpioIndexMax do
        if Bitwise.Extract(index, configuration) ==
            PIO.GpioConfigurationEnum.input then
            merge = merge | (Bitwise.Extract(i, input) << index)
        else
            merge = merge | (Bitwise.Extract(i, output) << index)
        end
    end
    return merge
end

function PIO:New(name, nodeId)
    local ib = Ib:New()
    ib.kind = 03;
    if name == nil then name = "pio" end
    ib.name = name
    if nodeId == nil then nodeId = ib.kind end
    ib.nodeId = nodeId
    ib.eds = "lua/core/cep/eds/pio_1.0.0.eds"
    return setmetatable({
        ib = ib,
        cache = {gpio = {polarity = 0, configuration = 0xFFFF}}
    }, PIO)
end

function PIO:LoadCache()
    self:Gpios()
    self:GpiosPolarity()
    self:GpiosConfiguration()
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

function PIO:GpioValues(value)
    local odIn = self.id.od["GPIO"]["Input Port"]
    local odOut = self.id.od["GPIO"]["Output Port"]
    if value == nil then
        local input = self.id:Upload(odIn)
        self.cache.gpio.output = self.id:Upload(odOut)
        return PIO.MergeGpioValues(input, output, self.cache.gpio.configuration)
    else
        self.cache.gpio.output = PIO.NormalizeGpioValueOutput(value, self.cache
                                                                  .gpio
                                                                  .configuration)
        self.id:Download(self.cache.gpio.output)
    end
end

function PIO:GpioValue(index, value)
    PIO.CheckGpioIndex(index)
    PIO.CheckGpioIndex(index)
    if value == nil then
        return Bitwise.Extract(index, PIO:GpioValues())
    else
        PIO:GpioValues(Bitwise.Inject(index, value, self.cache.gpio.output))
    end

end

function PIO:GpioPolarities(value)
    local od = self.id.od["GPIO"]["Polarity Inversion"]
    if value == nil then
        value = self.id.Upload(od)
        self.cache.gpio.polarity = value
        return value
    elseif IsUnsigned8(value) then
        self.cache.gpio.polarity = value
        self.id.Download(od, value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function PIO:GpioPolarity(index, value)
    PIO.CheckGpioIndex(index)
    if value == nil then
        return Bitwise.Extract(index, PIO:GpioPolarities())
    else
        PIO:GpioPolarities(Bitwise.Inject(index, value, self.cache.polarity))
    end
end

function PIO:GpioConfigurations(value)
    local od = self.id.od["GPIO"]["Configuration"]
    if value == nil then
        value = self.id:Upload(od)
        self.cache.gpio.configuration = value
        return value
    elseif IsUnsigned8(value) then
        self.cache.gpio.configuration = value
        self.id:Download(od, value)
    else
        error("Invalid value: " .. tostring(value))
    end
end

function PIO:GpioConfiguration(index, value)
    PIO.CheckGpioIndex(index)
    if value == nil then
        return Bitwise.Extract(index, PIO:GpioConfigurations())
    else
        PIO:GpioPolarities(
            Bitwise.Inject(index, value, self.cache.confgiuration))
    end
end
