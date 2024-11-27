local Ib = require("lua/core/sdk/environment/ib")
local IsBoolean = require("lua/core/utils/is_boolean")
local IsIntegerInOd = require("lua/core/utils/is_integer/is_integer_in_od")
local IsFloatInOd = require("lua/core/utils/is_float/is_float_in_od")
local IsUnsigned16 = require("lua/core/utils/is_unsigned/is_unsigned_16")
local IsUnsignedIn = require("lua/core/utils/is_unsigned/is_unsigned_in")
local Bitwise = require("lua/core/utils/bitwise")
local CheckField = require("lua/core/utils/check_field")

---@class PIO_CacheGpio
---@field output integer
---@field polarity integer
---@field mode integer

---@class PIO_Cache
---@field gpio PIO_CacheGpio

---@class PIO
---@field ib Ib?
---@field cache PIO_Cache
PIO = {
    ib = nil,
    cache = {
        gpio = {
            input = 0,
            output = 0,
            polarity = 0,
            mode = 0,
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
PIO.IoValuesMax = 0x0FFF

--- @enum PIO_IoPolarityEnum
PIO.IoPolarityEnum = { regular = 0, inverted = 1 }

--- @enum PIO_IoModeEnum
PIO.IoModeEnum = { input = 0, output = 1 }

--- @enum PIO_IoValueEnum
PIO.IoValueEnum = { low = 0, high = 1 }

--- Alias to PIO.IoValueEnum.high
PIO.HIGH = PIO.IoValueEnum.high
--- Alias to PIO.IoValueEnum.low
PIO.LOW = PIO.IoValueEnum.low

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
                mode = 0,
            }
        }
    }, PIO)
end

--- Reset the board
function PIO:Reset()
    self.ib:Reset()
end

--- Load current state of the board.
--- Populate cache entry for IO values, polarities and mode.
function PIO:LoadCache()
    self:IoOutputValues()
    self:IoPolarities()
    self:IoModes()
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
    return self.ib:Upload(self.ib.od[odName]["Current"]) --[[@as number]]
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
        return self.ib:Upload(od) --[[@as number]]
    else
        CheckField(voltage, "desired voltage", IsFloatInOd(voltage, od))
        self.ib:Download(od, voltage --[[@as OdEntryType]])
    end
end

--- Supply's actual voltage getter.
--- @param supply PIO_SupplyEnum
--- @return number voltage
function PIO:SupplyVoltage(supply)
    CheckSupplyEnum(supply)
    local odName = PIO.SupplyEnumToOdName(supply)
    return self.ib:Upload(self.ib.od[odName]["Voltage"]) --[[@as number]]
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
        return self.ib:Upload(od) --[[@as boolean]]
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
        return self.ib:Upload(od) --[[@as integer]]
    else
        CheckField(period, "Grace Period", IsIntegerInOd(period, od))
        self.ib:Download(od, period)
    end
end

--- All IO's input values getter
--- @return integer values bitpack of PIO_IoValueEnum
function PIO:IoInputValues()
    return self.ib:Upload(self.ib.od["IO"]["Input Port"]) --[[@as integer]]
end

--- Single IO's input value getter.
--- @param index integer
--- @return PIO_IoValueEnum
function PIO:IoInputValue(index)
    CheckIoIndex(index)
    return Bitwise.Extract(index, self:IoInputValues())
end

--- All IO's output values accessor.
--- if values is provided, function will act as setter and return nothing.
--- @param values? integer bitpack of PIO_IoValueEnum
--- @return integer? values bitpack of PIO_IoValueEnum
function PIO:IoOutputValues(values)
    local od = self.ib.od["IO"]["Output Port"]
    if values == nil then
        self.cache.gpio.output = self.ib:Upload(od) --[[@as integer]]
        return self.cache.gpio.output
    else
        CheckField(values, "values", IsUnsignedIn(values, 0, PIO.IoValuesMax))
        self.cache.gpio.output = values
        self.ib:Download(od, values)
    end
end

--- Single IO's output value accessor.
--- if value is provided, function will act as setter and return nothing.
--- @param index integer
--- @param value? PIO_IoValueEnum | integer
--- @return PIO_IoValueEnum? value
function PIO:IoOutputValue(index, value)
    CheckIoIndex(index)
    if value == nil then
        return Bitwise.Extract(index, self:IoOutputValues()) --[[@as PIO_IoValueEnum]]
    else
        CheckIoIndex(index)
        CheckField(value, "value", value == 0 or value == 1)
        self:IoOutputValues(Bitwise.Inject(index, value, self.cache.gpio.output))
    end
end

--- Multiple IO's polarities accessor.
--- if polarities is provided, function will act as setter and return nothing.
--- @param polarities integer? bitpack of PIO_IoPolarityEnum. 0=regular, 1=inverted
--- @return integer? polarities bitpack of PIO_IoPolarityEnum
function PIO:IoPolarities(polarities)
    local od = self.ib.od["IO"]["Polarity Inversion"]
    if polarities == nil then
        polarities = self.ib:Upload(od) --[[@as PIO_IoPolarityEnum]]
        self.cache.gpio.polarity = polarities
        return polarities
    else
        CheckField(polarities, "polarities", IsUnsigned16(polarities))
        self.cache.gpio.polarity = polarities
        self.ib:Download(od, polarities)
    end
end

--- Single IO's polarity accessor.
--- if value is provided, function will act as setter and return nothing.
--- @param index integer
--- @param polarity PIO_IoPolarityEnum?
--- @return PIO_IoPolarityEnum?
function PIO:IoPolarity(index, polarity)
    CheckIoIndex(index)
    if polarity == nil then
        return Bitwise.Extract(index, self:IoPolarities())
    else
        CheckIoIndex(index)
        CheckField(polarity, "polarity", polarity == 0 or polarity == 1)
        self:IoPolarities(Bitwise.Inject(index, polarity, self.cache.gpio.polarity))
    end
end

--- Multiple IO's modes accessor.
--- if modes is provided, function will act as setter and return nothing.
--- @param modes? integer bitpack of PIO_IoModeEnum. per bit: 0=input, 1=output
--- @return integer? modes bitpack of PIO_IoModeEnum
function PIO:IoModes(modes)
    local od = self.ib.od["IO"]["Mode"]
    if modes == nil then
        self.cache.gpio.mode = self.ib:Upload(od) --[[@as integer]]
        return self.cache.gpio.mode
    else
        CheckField(modes, "modes", IsUnsignedIn(modes, 0, PIO.IoValuesMax))
        self.cache.gpio.mode = modes
        self.ib:Download(od, modes)
    end
end

--- Single IO's configuration accessor.
--- if mode is provided, function will act as setter and return nothing.
--- @param index integer
--- @param mode PIO_IoModeEnum?
--- @return PIO_IoModeEnum?
function PIO:IoMode(index, mode)
    CheckIoIndex(index)
    if mode == nil then
        return Bitwise.Extract(index, self:IoModes())
    else
        CheckField(mode, "mode", mode == 0 or mode == 1)
        self:IoModes(Bitwise.Inject(index, mode, self.cache.gpio.mode))
    end
end
