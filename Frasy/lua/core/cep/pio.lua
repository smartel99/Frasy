local Ib = require("lua/core/sdk/environment/ib")
local IsBoolean = require("lua/core/utils/is_boolean")
local IsIntegerInOd = require("lua/core/utils/is_integer/is_integer_in_od")
local IsFloatInOd = require("lua/core/utils/is_float/is_float_in_od")
local IsUnsigned16 = require("lua/core/utils/is_unsigned/is_unsigned_16")
local IsUnsignedIn = require("lua/core/utils/is_unsigned/is_unsigned_in")
local Bitwise = require("lua/core/utils/bitwise")
local CheckField = require("lua/core/utils/check_field")

--- @class PIO
--- @field ib Ib?
PIO = {
    ib = nil,
    cache = {
        gpio = {
            input = 0,
            output = 0,
            polarity = 0,
            configuration = 0x0FFF,
        }
    }
}
PIO.__index = PIO

--- @enum PIO_SupplyEnum
PIO.SupplyEnum = {
    p3v3 = 1,
    p5v = 2,
    p12v = 3,
    p24v = 4,
    pVariable1 = 5,
    pVariable2 = 6
}

PIO.IoIndexMax = 11

--- @enum PIO_IoPolarityEnum
PIO.IoPolarityEnum = { regular = 0, inverted = 1 }

--- @enum PIO_IoConfigurationEnum
PIO.IoConfigurationEnum = { input = 0, output = 1 }

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

local function CheckIoIndex(index)
    CheckField(index, "gpio index", IsUnsignedIn(index, 0, PIO.IoIndexMax))
end

local function CheckIoValue(value)
    -- CheckField(value, "value", IsUnsignedIn(value, 0, 0x3FF))
    CheckField(value, "value", IsUnsigned16(value))
end

--- @class PIO_NewOptionalParameters
--- @field name string? default to "pio"
--- @field nodeId integer? default to 3

--- Instantiates a PIO.
--- Only to be used during environment declaration.
--- @param opt PIO_NewOptionalParameters?
--- @return PIO
function PIO:New(opt)
    local ib = Ib:New()
    ib.kind = 03;
    if opt == nil then
        opt = {}
    end
    CheckField(opt, "opt", type(opt) == "table")
    if opt.name == nil then
        opt.name = "pio"
    end
    if opt.nodeId == nil then
        opt.nodeId = ib.kind
    end
    ib.name = opt.name
    ib.nodeId = opt.nodeId
    ib.eds = "lua/core/cep/eds/pio_1.0.0.eds"
    return setmetatable({
        ib = ib,
        cache = {
            gpio = {
                output = 0,
                polarity = 0,
                configuration = 0x3FF,
            }
        }
    }, PIO)
end

--- Reset the board
function PIO:Reset()
    self.ib:Reset()
end

--- Load current state of the board.
--- Populate cache entry for IO values, polarities and configurations.
function PIO:LoadCache()
    self:IoInputValues()
    self:IoOutputValues()
    self:IoPolarities()
    self:IoConfigurations()
end

--- Supply's Current Limit Accessor.
--- If limit is provided, function will be act as setter and return nothing.
--- @param supply PIO_SupplyEnum
--- @param limit integer?
--- @return number? limit
function PIO:SupplyCurrentLimit(supply, limit)
    CheckSupplyEnum(supply)
    local odName = PIO.SupplyEnumToOdName(supply)
    local od = self.ib.od[odName]["Current Limit"]
    if limit ~= nil then
        CheckField(limit, "current limit", IsIntegerInOd(limit, od))
        self.ib:Download(od, limit)
    else
        return self.ib:Upload(od) --[[@as number]]
    end
end

--- Supply's actual current usage getter.
--- @param supply PIO_SupplyEnum
--- @return number current
function PIO:SupplyCurrent(supply)
    CheckSupplyEnum(supply)
    local odName = PIO.SupplyEnumToOdName(supply)
    return self.ib:Upload(self.ib.od[odName]["Current"])
end

--- Variable supply's voltage accessor.
--- If voltage is provided, function will be act as setter and return nothing.
--- @param supply PIO_SupplyEnum variable supply only
--- @param voltage number?
--- @return number? voltage
function PIO:SupplyDesiredVoltage(supply, voltage)
    CheckVariableSupplyEnum(supply)
    local odName = PIO.SupplyEnumToOdName(supply)
    local od = self.ib.od[odName]["Desired Voltage"]
    if voltage == nil then
        return self.ib:Upload(od)
    else
        CheckField(voltage, "desired voltage", IsFloatInOd(voltage, od))
        self.ib:Download(od, voltage)
    end
end

--- Supply's actual voltage getter.
--- @param supply PIO_SupplyEnum
--- @return number voltage
function PIO:SupplyVoltage(supply)
    CheckSupplyEnum(supply)
    local odName = PIO.SupplyEnumToOdName(supply)
    return self.ib:Upload(self.ib.od[odName]["Voltage"])
end

--- Supply's enable accessor.
--- If state is provided, function will act as setter and return nothing.
--- @param supply PIO_SupplyEnum
--- @param state boolean?
--- @return boolean? state
function PIO:SupplyOutputEnable(supply, state)
    CheckSupplyEnum(supply)
    local odName = PIO.SupplyEnumToOdName(supply)
    local od = self.ib.od[odName]["Output Enable"]
    if state == nil then
        return self.ib:Upload(od)
    else
        CheckField(state, "output enable", IsBoolean(state))
        self.ib:Download(od, state)
    end
end

--- Supply's grace period accessor.
--- If period is provided, function will act as a setter and return nothing.
---@param supply PIO_SupplyEnum
---@param period integer? Period, in ticks (~1ms) during which the output voltage of a supply will not be checked after a transition in state.
---A value of 0 disables the grace period.
---A value of -1 disables the monitoring of the output voltage.
---
---@return integer? the current grace period.
function PIO:SupplyGracePeriod(supply, period)
    CheckSupplyEnum(supply)
    local odName = PIO.SupplyEnumToOdName(supply)
    local od = self.ib.od[odName]["Grace Period"]
    if period == nil then
        return self.ib:Upload(od)
    else
        CheckField(period, "Grace Period", IsIntegerInOd(period, od))
        self.ib:Download(od, period)
    end
end

--- All IO's input values accessor.
--- if values is provided, function will act as setter and return nothing.
--- @param values? integer
--- @return integer? values
function PIO:IoInputValues(values)
    local od = self.ib.od["IO"]["Input Port"]
    if values == nil then
        local input = self.ib:Upload(od)
        self.cache.gpio.input = input
        return input
    else
        CheckIoValue(values)
        self.cache.gpio.input = values
        self.ib:Download(od, self.cache.gpio.input)
    end
end

--- Single IO's input value accessor.
--- if value is provided, function will act as setter and return nothing.
--- @param index integer
--- @param value boolean?
--- @return boolean? value
function PIO:IoInputValue(index, value)
    CheckIoIndex(index)
    if value == nil then
        return Bitwise.Extract(index, self:IoInputValues())
    else
        CheckIoValue(value)
        self:IoInputValues(Bitwise.Inject(index, value, self.cache.gpio.input))
    end
end

--- All IO's output values accessor.
--- if values is provided, function will act as setter and return nothing.
--- @param values? integer
--- @return integer? values
function PIO:IoOutputValues(values)
    local od = self.ib.od["IO"]["Output Port"]
    if values == nil then
        local input = self.ib:Upload(od)
        self.cache.gpio.output = input
        return input
    else
        CheckIoValue(values)
        self.cache.gpio.output = values
        self.ib:Download(od, self.cache.gpio.output)
    end
end

--- Single IO's output value accessor.
--- if value is provided, function will act as setter and return nothing.
--- @param index integer
--- @param value boolean?
--- @return boolean? value
function PIO:IoOutputValue(index, value)
    CheckIoIndex(index)
    if value == nil then
        return Bitwise.Extract(index, self:IoOutputValues())
    else
        CheckIoValue(value)
        self:IoOutputValues(Bitwise.Inject(index, value, self.cache.gpio.output))
    end
end

--- Multiple IO's polarities accessor.
--- if values is provided, function will act as setter and return nothing.
--- @param values integer? array of bit polarities. 0=regular, 1=inverted
--- @return PIO_IoPolarityEnum? values
function PIO:IoPolarities(values)
    local od = self.ib.od["IO"]["Polarity Inversion"]
    if values == nil then
        values = self.ib:Upload(od)
        self.cache.gpio.polarity = values
        return values
    else
        CheckIoValue(values)
        self.cache.gpio.polarity = values
        self.ib:Download(od, values)
    end
end

--- Single IO's polarity accessor.
--- if value is provided, function will act as setter and return nothing.
--- @param index integer
--- @param value PIO_IoPolarityEnum?
--- @return PIO_IoPolarityEnum? value
function PIO:IoPolarity(index, value)
    CheckIoIndex(index)
    if value == nil then
        return Bitwise.Extract(index, self:IoPolarities())
    else
        CheckIoValue(value)
        self:IoPolarities(Bitwise.Inject(index, value,
                self.cache.gpio.polarity))
    end
end

--- Multiple IO's configurations accessor.
--- if values is provided, function will act as setter and return nothing.
--- @param values? integer array of configurations. per bit: 0=input, 1=output
--- @return integer? values
function PIO:IoConfigurations(values)
    local od = self.ib.od["IO"]["Configuration"]
    if values == nil then
        values = self.ib:Upload(od)
        self.cache.gpio.configuration = values
        return values
    else
        CheckIoValue(values)
        self.cache.gpio.configuration = values
        self.ib:Download(od, values)
    end
end

--- Single IO's configuration accessor.
--- if value is provided, function will act as setter and return nothing.
--- @param index integer
--- @param value PIO_IoConfigurationEnum?
--- @return PIO_IoConfigurationEnum? value
function PIO:GpioConfiguration(index, value)
    CheckIoIndex(index)
    if value == nil then
        return Bitwise.Extract(index, self:IoConfigurations())
    else
        CheckIoValue(value)
        self:IoConfigurations(Bitwise.Inject(index, value,
                self.cache.gpio.configuration))
    end
end
