local Ib = require("lua/core/sdk/environment/ib")
local IsBoolean = require("lua/core/utils/is_boolean")
local IsIntegerInOd = require("lua/core/utils/is_integer_in_od")
local IsFloatInOd = require("lua/core/utils/is_float/is_float_in_od")
local IsUnsigned16 = require("lua/core/utils/is_unsigned/is_unsigned_16")
local IsUnsignedIn = require("lua/core/utils/is_unsigned/is_unsigned_in")
local Bitwise = require("lua/core/utils/bitwise")
local CheckField = require("lua/core/utils/check_field")

---@class PIO
---@field ib Ib?
PIO = {
    ib = nil,
    cache = {
        gpio = { input = 0, output = 0, polarity = 0, configuration = 0x0FFF }
    }
}
PIO.__index = PIO

PIO.SupplyEnum = {
    p3v3 = 1,
    p5v = 2,
    p12v = 3,
    p24v = 4,
    pVariable1 = 5,
    pVariable2 = 6
}

PIO.GpioIndexMax = 11

PIO.GpioPolarityEnum = { regular = 0, inverted = 1 }

PIO.GpioConfigurationEnum = { output = 0, input = 1 }

local function CheckSupplyEnum(supply)
    CheckField(supply, "supply", IsUnsignedIn(supply, PIO.SupplyEnum.p3v3,
        PIO.SupplyEnum.pVariable2))
end

local function CheckVariableSupplyEnum(supply)
    CheckField(supply, "supply", IsUnsignedIn(supply, PIO.SupplyEnum.pVariable1,
        PIO.SupplyEnum.pVariable2))
end

function PIO.SupplyEnumToOdName(supply)
    local names = {
        "Supply 3V3", "Supply 5V", "Supply 12V", "Supply 24V",
        "Variable Supply 1", "Variable Supply 2"
    }
    return names[supply]
end

local function CheckGpioIndex(index)
    CheckField(index, "gpio index", IsUnsignedIn(index, 0, PIO.GpioIndexMax))
end

local function CheckGpioValue(value)
    -- CheckField(value, "value", IsUnsignedIn(value, 0, 0x3FF))
    CheckField(value, "value", IsUnsigned16(value)) -- Because OD is badly set
end

local function NormalizeGpioValueOutput(value, configuration)
    for index = 0, PIO.GpioIndexMax do
        if Bitwise.Extract(index, configuration) ==
            PIO.GpioConfigurationEnum.output then
            value = Bitwise.Inject(index, Bitwise.Extract(index, value), value)
        else
            value = Bitwise.Inject(index, 0, value)
        end
    end
    return value
end

local function MergeGpioValues(input, output, configuration)
    local merge = 0
    for index = 0, PIO.GpioIndexMax do
        local field
        if Bitwise.Extract(index, configuration) ==
            PIO.GpioConfigurationEnum.input then
            field = input
        else
            field = output
        end
        merge = merge | (Bitwise.Extract(index, field) << index)
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
        cache = { gpio = { polarity = 0, configuration = 0x3FF } }
    }, PIO)
end

function PIO:LoadCache()
    self:GpioValues()
    self:GpioPolarities()
    self:GpioConfigurations()
end

function PIO:CurrentLimit(supply, value)
    CheckSupplyEnum(supply)
    local odName = PIO.SupplyEnumToOdName(supply)
    local od = self.ib.od[odName]["Current Limit"]
    if value == nil then
        return self.ib:Upload(od)
    else
        CheckField(value, "current limit", IsIntegerInOd(value, od))
        self.ib:Download(od, value)
    end
end

function PIO:Current(supply)
    CheckSupplyEnum(supply)
    local odName = PIO.SupplyEnumToOdName(supply)
    return self.ib:Upload(self.ib.od[odName]["Current"])
end

function PIO:DesiredVoltage(supply, value)
    CheckVariableSupplyEnum(supply)
    local odName = PIO.SupplyEnumToOdName(supply)
    local od = self.ib.od[odName]["Desired Voltage"]
    if value == nil then
        return self.ib:Upload(od)
    else
        CheckField(value, "desired voltage", IsFloatInOd(value, od))
        self.ib:Download(od, value)
    end
end

function PIO:Voltage(supply)
    CheckSupplyEnum(supply)
    local odName = PIO.SupplyEnumToOdName(supply)
    return self.ib:Upload(self.ib.od[odName]["Voltage"])
end

function PIO:OutputEnable(supply, value)
    CheckSupplyEnum(supply)
    local odName = PIO.SupplyEnumToOdName(supply)
    local od = self.ib.od[odName]["Output Enable"]
    if value == nil then
        return self.ib:Upload(od)
    else
        CheckField(value, "output enable", IsBoolean(value))
        self.ib:Download(od, value)
    end
end

function PIO:GpioValues(value)
    local odIn = self.ib.od["GPIO"]["Input Port"]
    local odOut = self.ib.od["GPIO"]["Output Port"]
    if value == nil then
        local input = self.ib:Upload(odIn)
        local output = self.ib:Upload(odOut)
        self.cache.gpio.output = output
        return MergeGpioValues(input, output, self.cache.gpio.configuration)
    else
        CheckGpioValue(value)
        self.cache.gpio.output = NormalizeGpioValueOutput(value, self.cache.gpio
            .configuration)
        self.ib:Download(odOut, self.cache.gpio.output)
    end
end

function PIO:GpioValue(index, value)
    CheckGpioIndex(index)
    if value == nil then
        return Bitwise.Extract(index, self:GpioValues())
    else
        CheckGpioValue(value)
        self:GpioValues(Bitwise.Inject(index, value, self.cache.gpio.output))
    end
end

function PIO:GpioPolarities(value)
    local od = self.ib.od["GPIO"]["Polarity Inversion"]
    if value == nil then
        value = self.ib:Upload(od)
        self.cache.gpio.polarity = value
        return value
    else
        CheckGpioValue(value)
        self.cache.gpio.polarity = value
        self.ib:Download(od, value)
    end
end

function PIO:GpioPolarity(index, value)
    CheckGpioIndex(index)
    if value == nil then
        return Bitwise.Extract(index, self:GpioPolarities())
    else
        CheckGpioValue(value)
        self:GpioPolarities(Bitwise.Inject(index, value,
            self.cache.gpio.polarity))
    end
end

function PIO:GpioConfigurations(value)
    local od = self.ib.od["GPIO"]["Configuration"]
    if value == nil then
        value = self.ib:Upload(od)
        self.cache.gpio.configuration = value
        return value
    else
        CheckGpioValue(value)
        self.cache.gpio.configuration = value
        self.ib:Download(od, value)
    end
end

function PIO:GpioConfiguration(index, value)
    CheckGpioIndex(index)
    if value == nil then
        return Bitwise.Extract(index, self:GpioConfigurations())
    else
        CheckGpioValue(value)
        self:GpioConfigurations(Bitwise.Inject(index, value,
            self.cache.gpio.configuration))
    end
end
