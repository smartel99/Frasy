--- @type Ib
local Ib = require("lua/core/sdk/environment/ib")
local Bitwise = require("lua/core/utils/bitwise")
local IsBoolean = require("lua/core/utils/is_boolean")
local IsInteger = require("lua/core/utils/is_integer/is_integer")
local IsIntegerIn = require("lua/core/utils/is_integer/is_integer_in")
local IsIntegerInOd = require("lua/core/utils/is_integer/is_integer_in_od")
local IsUnsignedIn = require("lua/core/utils/is_unsigned/is_unsigned_in")
local IsUnsignedInOd = require("lua/core/utils/is_unsigned/is_unsigned_in_od")
local IsFloatInOd = require("lua/core/utils/is_float/is_float_in_od")
local TimeoutFunction = require("lua/core/utils/timeout_function")
local CheckField = require("lua/core/utils/check_field")
local StringizeValues = require("lua/core/utils/stringize_values")
local TryFunction = require("lua/core/utils/try_function")

--- @class DAQ_CacheIo
--- @field mode integer
--- @field output integer

--- @class DAQ_Cache
--- @field io DAQ_CacheIo

--- @class DAQ
--- @field ib? Ib
--- @field cache DAQ_Cache
DAQ = { ib = nil, cache = { io = { mode = 0, output = 0, } } }
DAQ.__index = DAQ

--- @class DAQ_NewOptionalParameters
--- @field name string? default to "daq"
--- @field nodeId integer? default to 2

--- Instantiates a DAQ.
--- Only to be used during environment declaration.
--- @param opt DAQ_NewOptionalParameters?
--- @return DAQ
function DAQ:New(opt)
    local ib = Ib:New()
    ib.kind = 02;
    if opt == nil then opt = {} end
    CheckField(opt, "opt", type(opt) == "table")
    if opt.name == nil then opt.name = "daq" end
    if opt.nodeId == nil then opt.nodeId = ib.kind end
    ib.name = opt.name
    ib.nodeId = opt.nodeId
    ib.eds = "lua/core/cep/eds/daq.eds"
    return setmetatable({ ib = ib, cache = { io = { mode = 0, output = 0, } } }, DAQ)
end

function DAQ:Reset()
    self.ib:Reset()
end

function DAQ:LoadCache()
    self:IoModes()
    self:IoOutputValues()
end

-- Routing
--- @enum DAQ_RoutingBusEnum
DAQ.RoutingBusEnum = { all = 0, bus1 = 1, bus2 = 2, bus3 = 3, bus4 = 4 }

--- @enum DAQ_RoutingPointsEnum
DAQ.RoutingPointsEnum = {
    -- FORMAT COMPLY TO XDD
    NONE = 0,
    MUX1_A0 = 1,
    MUX1_A1 = 2,
    MUX1_A2 = 3,
    MUX1_A3 = 4,
    MUX1_B0 = 5,
    MUX1_B1 = 6,
    MUX1_B2 = 7,
    MUX1_B3 = 8,
    MUX1_OUT = 9,
    MUX2_A0 = 10,
    MUX2_A1 = 11,
    MUX2_A2 = 12,
    MUX2_A3 = 13,
    MUX2_B0 = 14,
    MUX2_B1 = 15,
    MUX2_B2 = 16,
    MUX2_B3 = 17,
    MUX2_OUT = 18,
    MUX3_A0 = 19,
    MUX3_A1 = 20,
    MUX3_A2 = 21,
    MUX3_A3 = 22,
    MUX3_B0 = 23,
    MUX3_B1 = 24,
    MUX3_B2 = 25,
    MUX3_B3 = 26,
    MUX3_OUT = 27,
    MUX4_A0 = 28,
    MUX4_A1 = 29,
    MUX4_A2 = 30,
    MUX4_A3 = 31,
    MUX4_B0 = 32,
    MUX4_B1 = 33,
    MUX4_B2 = 34,
    MUX4_B3 = 35,
    MUX4_OUT = 36,
    MUX5_A0 = 37,
    MUX5_A1 = 38,
    MUX5_A2 = 39,
    MUX5_A3 = 40,
    MUX5_B0 = 41,
    MUX5_B1 = 42,
    MUX5_B2 = 43,
    MUX5_B3 = 44,
    MUX5_OUT = 45,
    MUX6_A0 = 46,
    MUX6_A1 = 47,
    MUX6_A2 = 48,
    MUX6_A3 = 49,
    MUX6_B0 = 50,
    MUX6_B1 = 51,
    MUX6_B2 = 52,
    MUX6_B3 = 53,
    MUX6_OUT = 54,
    R100_1 = 55,
    R100_2 = 56,  -- This is the same as `R100_1`, but is required internally.
    R4k99_1 = 57,
    R4k99_2 = 58, -- This is the same as `R4k99_1`, but is required internally.
    R100k_1 = 59,
    R100k_2 = 60, -- This is the same as `R100k_1`, but is required internally.
    R1M_1 = 61,
    R1M_2 = 62,   -- This is the same as `R1M_1`, but is required internally.
    P3V3 = 63,
    P5V = 64,
    P2V048 = 65,
    P24V = 66,
    GND = 67,
    DAC = 68,
    BUS_IO_A = 69,
    BUS_IO_B = 70,
    ADC_CH1 = 71, -- Corresponds to `SIG_ADC_2` in the schematics.
    ADC_CH2 = 72, -- Corresponds to `SIG_ADC_3` in the schematics.
    IMP_P = 73,
    IMP_N = 74,
    GUARD = 75,
    BUS = 76, -- Debug only. Allows to connect single point to bus
}

function DAQ.RoutingBusEnumToString(bus)
    if not IsIntegerIn(bus, DAQ.RoutingBusEnum.all, DAQ.RoutingBusEnum.bus4) then
        error("Invalid bus: " .. tostring(bus))
    end
    local names = {
        [DAQ.RoutingBusEnum.all] = "All",
        [DAQ.RoutingBusEnum.bus1] = "Bus 1",
        [DAQ.RoutingBusEnum.bus2] = "Bus 2",
        [DAQ.RoutingBusEnum.bus3] = "Bus 3",
        [DAQ.RoutingBusEnum.bus4] = "Bus 4"
    }
    return names[bus]
end

function DAQ.RoutingPointsEnumToString(point)
    CheckField(point, "point", IsIntegerIn(point, DAQ.RoutingPointsEnum.NONE, DAQ.RoutingPointsEnum.GUARD))
    local names = {
        [DAQ.RoutingPointsEnum.NONE] = "NONE",
        [DAQ.RoutingPointsEnum.MUX1_A0] = "MUX1_A0",
        [DAQ.RoutingPointsEnum.MUX1_A1] = "MUX1_A1",
        [DAQ.RoutingPointsEnum.MUX1_A2] = "MUX1_A2",
        [DAQ.RoutingPointsEnum.MUX1_A3] = "MUX1_A3",
        [DAQ.RoutingPointsEnum.MUX1_B0] = "MUX1_B0",
        [DAQ.RoutingPointsEnum.MUX1_B1] = "MUX1_B1",
        [DAQ.RoutingPointsEnum.MUX1_B2] = "MUX1_B2",
        [DAQ.RoutingPointsEnum.MUX1_B3] = "MUX1_B3",
        [DAQ.RoutingPointsEnum.MUX1_OUT] = "MUX1_OUT",
        [DAQ.RoutingPointsEnum.MUX2_A0] = "MUX2_A0",
        [DAQ.RoutingPointsEnum.MUX2_A1] = "MUX2_A1",
        [DAQ.RoutingPointsEnum.MUX2_A2] = "MUX2_A2",
        [DAQ.RoutingPointsEnum.MUX2_A3] = "MUX2_A3",
        [DAQ.RoutingPointsEnum.MUX2_B0] = "MUX2_B0",
        [DAQ.RoutingPointsEnum.MUX2_B1] = "MUX2_B1",
        [DAQ.RoutingPointsEnum.MUX2_B2] = "MUX2_B2",
        [DAQ.RoutingPointsEnum.MUX2_B3] = "MUX2_B3",
        [DAQ.RoutingPointsEnum.MUX2_OUT] = "MUX2_OUT",
        [DAQ.RoutingPointsEnum.MUX3_A0] = "MUX3_A0",
        [DAQ.RoutingPointsEnum.MUX3_A1] = "MUX3_A1",
        [DAQ.RoutingPointsEnum.MUX3_A2] = "MUX3_A2",
        [DAQ.RoutingPointsEnum.MUX3_A3] = "MUX3_A3",
        [DAQ.RoutingPointsEnum.MUX3_B0] = "MUX3_B0",
        [DAQ.RoutingPointsEnum.MUX3_B1] = "MUX3_B1",
        [DAQ.RoutingPointsEnum.MUX3_B2] = "MUX3_B2",
        [DAQ.RoutingPointsEnum.MUX3_B3] = "MUX3_B3",
        [DAQ.RoutingPointsEnum.MUX3_OUT] = "MUX3_OUT",
        [DAQ.RoutingPointsEnum.MUX4_A0] = "MUX4_A0",
        [DAQ.RoutingPointsEnum.MUX4_A1] = "MUX4_A1",
        [DAQ.RoutingPointsEnum.MUX4_A2] = "MUX4_A2",
        [DAQ.RoutingPointsEnum.MUX4_A3] = "MUX4_A3",
        [DAQ.RoutingPointsEnum.MUX4_B0] = "MUX4_B0",
        [DAQ.RoutingPointsEnum.MUX4_B1] = "MUX4_B1",
        [DAQ.RoutingPointsEnum.MUX4_B2] = "MUX4_B2",
        [DAQ.RoutingPointsEnum.MUX4_B3] = "MUX4_B3",
        [DAQ.RoutingPointsEnum.MUX4_OUT] = "MUX4_OUT",
        [DAQ.RoutingPointsEnum.MUX5_A0] = "MUX5_A0",
        [DAQ.RoutingPointsEnum.MUX5_A1] = "MUX5_A1",
        [DAQ.RoutingPointsEnum.MUX5_A2] = "MUX5_A2",
        [DAQ.RoutingPointsEnum.MUX5_A3] = "MUX5_A3",
        [DAQ.RoutingPointsEnum.MUX5_B0] = "MUX5_B0",
        [DAQ.RoutingPointsEnum.MUX5_B1] = "MUX5_B1",
        [DAQ.RoutingPointsEnum.MUX5_B2] = "MUX5_B2",
        [DAQ.RoutingPointsEnum.MUX5_B3] = "MUX5_B3",
        [DAQ.RoutingPointsEnum.MUX5_OUT] = "MUX5_OUT",
        [DAQ.RoutingPointsEnum.MUX6_A0] = "MUX6_A0",
        [DAQ.RoutingPointsEnum.MUX6_A1] = "MUX6_A1",
        [DAQ.RoutingPointsEnum.MUX6_A2] = "MUX6_A2",
        [DAQ.RoutingPointsEnum.MUX6_A3] = "MUX6_A3",
        [DAQ.RoutingPointsEnum.MUX6_B0] = "MUX6_B0",
        [DAQ.RoutingPointsEnum.MUX6_B1] = "MUX6_B1",
        [DAQ.RoutingPointsEnum.MUX6_B2] = "MUX6_B2",
        [DAQ.RoutingPointsEnum.MUX6_B3] = "MUX6_B3",
        [DAQ.RoutingPointsEnum.MUX6_OUT] = "MUX6_OUT",
        [DAQ.RoutingPointsEnum.R100_1] = "R100_1",
        [DAQ.RoutingPointsEnum.R100_2] = "R100_2",   -- This is the same as `R100_1`, but is required internally.
        [DAQ.RoutingPointsEnum.R4k99_1] = "R4k99_1",
        [DAQ.RoutingPointsEnum.R4k99_2] = "R4k99_2", -- This is the same as `R4k99_1`, but is required internally.
        [DAQ.RoutingPointsEnum.R100k_1] = "R100k_1",
        [DAQ.RoutingPointsEnum.R100k_2] = "R100k_2", -- This is the same as `R100k_1`, but is required internally.
        [DAQ.RoutingPointsEnum.R1M_1] = "R1M_1",
        [DAQ.RoutingPointsEnum.R1M_2] = "R1M_2",     -- This is the same as `R1M_1`, but is required internally.
        [DAQ.RoutingPointsEnum.P3V3] = "P3V3",
        [DAQ.RoutingPointsEnum.P5V] = "P5V",
        [DAQ.RoutingPointsEnum.P2V048] = "P2V048",
        [DAQ.RoutingPointsEnum.P24V] = "P24V",
        [DAQ.RoutingPointsEnum.GND] = "GND",
        [DAQ.RoutingPointsEnum.DAC] = "DAC",
        [DAQ.RoutingPointsEnum.BUS_IO_A] = "BUS_IO_A",
        [DAQ.RoutingPointsEnum.BUS_IO_B] = "BUS_IO_B",
        [DAQ.RoutingPointsEnum.ADC_CH1] = "ADC_CH1", -- Corresponds to `SIG_ADC_2` in the schematics.
        [DAQ.RoutingPointsEnum.ADC_CH2] = "ADC_CH2", -- Corresponds to `SIG_ADC_3` in the schematics.
        [DAQ.RoutingPointsEnum.IMP_P] = "IMP_P",
        [DAQ.RoutingPointsEnum.IMP_N] = "IMP_N",
        [DAQ.RoutingPointsEnum.GUARD] = "GUARD",
        [DAQ.RoutingPointsEnum.BUS] = "BUS",
    }
    return names[point]
end

local function _requestRouting(daq, points)
    daq.ib:Download(daq.ib.od["Routing"]["Request Routing"], points)
    SleepFor(35) -- Routing takes about 33ms on the DAQ
    return daq.ib:Upload(daq.ib.od["Routing"]["Last Result"]) --[[@as DAQ_RoutingBusEnum]]
end

local function _checkRouting(daq, points, bus)
    if bus == nil or bus < 1 or bus > 4 then return false end
    if #points < 2 then error("Invalid null-terminated string of points") end
    local state = daq:GetBusState(bus)
    if #state < #points then return false end
    for i = 1, #points - 1 do
        if state:byte(i, i) ~= points:byte(i, i) then
            return false
        end
    end
    return true
end

--- Requests a connection between different points of the board.
--- @param points DAQ_RoutingPointsEnum[]
--- @return DAQ_RoutingBusEnum
function DAQ:RequestRouting(points)
    if type(points) ~= "table" then error("Invalid points: " .. tostring(points)) end
    if #points < 2 then error("Invalid points count: " .. tostring(#points)) end
    local sPoints = {}
    for k, v in ipairs(points) do
        if not IsInteger(v) then error("Invalid point: [" .. k .. "] " .. tostring(v)) end
        table.insert(sPoints, v)
    end
    table.insert(sPoints, 0)
    if Context.info.stage ~= Stage.execution then return 0 end
    local sPointsAsStr = StringizeValues(table.unpack(sPoints))
    local route = nil
    TryFunction(function()
        route = _requestRouting(self, sPointsAsStr)
        -- SleepFor(1)
        return _checkRouting(self, sPointsAsStr, route)
    end, 5)
    return route --[[@as DAQ_RoutingBusEnum]]
end

--- Gets the state of a bus.
--- @param bus DAQ_RoutingBusEnum
--- @return DAQ_RoutingPointsEnum[]
function DAQ:GetBusState(bus)
    if not IsIntegerIn(bus, DAQ.RoutingBusEnum.bus1, DAQ.RoutingBusEnum.bus4) then
        error("Invalid bus: " .. tostring(bus))
    end
    if Context.info.stage ~= Stage.execution then return {} end
    return self.ib:Upload(self.ib.od["Routing"][DAQ.RoutingBusEnumToString(bus) .. " State"]) --[[@as DAQ_RoutingPointsEnum[] ]]
end

--- Clears a bus.
--- @param bus DAQ_RoutingBusEnum
function DAQ:ClearBus(bus)
    if not IsIntegerIn(bus, DAQ.RoutingBusEnum.all, DAQ.RoutingBusEnum.bus4) then
        error("Invalid bus: " .. tostring(bus))
    end
    self.ib:Download(self.ib.od["Routing"]["Clear " .. DAQ.RoutingBusEnumToString(bus)], true)
end

-- DAC
--- @enum DAQ_DacShape
DAQ.DacShapeEnum = { dc = 1, sine = 2, sawtooth = 3, triangle = 4, square = 5, noise = 6 }

--- Toggles the DAC, or get the current state of the DAC.
--- @param state? boolean The desired state.
--- @return boolean? state
function DAQ:DacEnable(state)
    local od = self.ib.od["DAC"]["Enable"]
    if state == nil then
        return self.ib:Upload(od) --[[@as boolean]]
    else
        CheckField(state, "enable", IsBoolean(state))
        self.ib:Download(od, state)
    end
end

--- Gets or sets the amplitude of the DAC, in volt.
--- @param amplitude? number
--- @return number? amplitude
function DAQ:DacAmplitude(amplitude)
    local od = self.ib.od["DAC"]["Amplitude"]
    if amplitude == nil then
        return self.ib:Upload(od) --[[@as number]]
    else
        CheckField(amplitude, "amplitude", IsFloatInOd(amplitude, od))
        self.ib:Download(od, amplitude)
    end
end

--- Sets the frequency of the DAC, or gets the current frequency of the DAC.
--- @param frequency? integer
--- @return number?
function DAQ:DacFrequency(frequency)
    local od = self.ib.od["DAC"]["Frequency"]
    if frequency == nil then
        return self.ib:Upload(od) --[[@as number]]
    else
        CheckField(frequency, "frequency", IsUnsignedInOd(frequency, od))
        self.ib:Download(od, frequency)
    end
end

--- Gets or set the shape of the waveform being output on the DAC.
--- @param shape? DAQ_DacShape
--- @return DAQ_DacShape?
function DAQ:DacShape(shape)
    local od = self.ib.od["DAC"]["Shape"]
    if shape == nil then
        return self.ib:Upload(od) --[[@as DAQ_DacShape]]
    else
        CheckField(shape, "shape", IsUnsignedIn(shape, DAQ.DacShapeEnum.dc, DAQ.DacShapeEnum.noise))
        self.ib:Download(od, shape)
    end
end

-- IO
DAQ.IoValuesMax = 0x3FF
DAQ.IoCount = 10

--- @enum DAQ_IoEnum
DAQ.IoEnum = {
    a = 0,
    b = 1,
    spiUutSck = 2,
    spiUutMosi = 3,
    spiUutMiso = 4,
    i2cUutScl = 5,
    i2cUutSda = 6,
    uartUutTx = 7,
    uartUutRx = 8,
    db9SpareIo = 9,
}

--- @enum DAQ_IoModeEnum
DAQ.IoModeEnum = { input = 0, output = 1 }

--- @enum DAQ_IoValueEnum
DAQ.IoValueEnum = { low = 0, high = 1 }

DAQ.HIGH = DAQ.IoValueEnum.high
DAQ.LOW = DAQ.IoValueEnum.low

local function CheckIoIndex(index)
    CheckField(index, "io index", IsIntegerIn(index, DAQ.IoEnum.a, DAQ.IoEnum.db9SpareIo))
end

--- Multiple IO mode accessor.
--- If modes is provided, function will act as setter
--- @param modes? integer bitpack of DAQ_IoModeEnum. per bit: 1=input, 0=output
--- @return integer? modes bitpack of DAQ_IoModeEnum
function DAQ:IoModes(modes)
    local od = self.ib.od["IO"]["Mode"]
    if (modes == nil) then
        self.cache.io.mode = self.ib:Upload(od) --[[@as integer]]
        return self.cache.io.mode
    else
        CheckField(modes, "modes", IsIntegerIn(modes, 0, DAQ.IoValuesMax))
        self.cache.io.mode = modes
        self.ib:Download(od, modes)
    end
end

--- Single IO mode accessor.
--- If mode is provided, function will act as setter
--- @param io DAQ_IoEnum
--- @param mode? DAQ_IoModeEnum
--- @return DAQ_IoModeEnum?
function DAQ:IoMode(io, mode)
    CheckIoIndex(io)
    if mode == nil then
        return Bitwise.Extract(io, self:IoModes())
    else
        CheckField(mode, "mode", mode == 0 or mode == 1)
        self:IoModes(Bitwise.Inject(io, mode, self.cache.io.mode))
    end
end

--- All IO's input values getter
--- @return integer values bitpack of DAQ_IoValueEnum
function DAQ:IoInputValues()
    local od = self.ib.od["IO"]["Input Port"]
    return self.ib:Upload(od) --[[@as integer]]
end

--- Single IO's input value getter.
--- @param index integer
--- @return DAQ_IoValueEnum
function DAQ:IoInputValue(index)
    CheckIoIndex(index)
    return Bitwise.Extract(index, self:IoInputValues())
end

--- All IO's output values accessor.
--- if values is provided, function will act as setter and return nothing.
--- @param values? integer bitpack of DAQ_IoValueEnum
--- @return integer? values bitpack of DAQ_IoValueEnum
function DAQ:IoOutputValues(values)
    local od = self.ib.od["IO"]["Output Port"]
    if (values == nil) then
        self.cache.io.output = self.ib:Upload(od) --[[@as integer]]
        return self.cache.io.output
    else
        CheckField(values, "values", IsUnsignedIn(values, 0, DAQ.IoValuesMax))
        self.cache.io.output = values
        self.ib:Download(od, values)
    end
end

--- Single value accessor.
--- If value is provided, function will act as setter
--- @param index DAQ_IoEnum
--- @param value DAQ_IoValueEnum?
--- @return DAQ_IoValueEnum?
function DAQ:IoOutputValue(index, value)
    CheckIoIndex(index)
    if value == nil then
        return Bitwise.Extract(index, self:IoOutputValues())
    else
        CheckField(value, "value", value == 0 or value == 1)
        self:IoOutputValues(Bitwise.Inject(index, value, self.cache.io.output))
    end
end

-- Signaling
--- @enum DAQ_SignalingModeEnum
DAQ.SignalingModeEnum = {
    off = 0,
    idle = 1,
    standby = 2,
    busy = 3,
    concern = 4,
    lockOut = 5,
    danger = 6,
    mutedDanger = 7,
    boot = 8,
    custom = 9
}
--- @enum DAQ_SignalingModeModifierEnum
DAQ.SignalingModeModifierEnum = {
    off = 0,
    on100msPer3s = 1,
    on100msPer1s = 2,
    on250msPer1s = 3,
    on500msPer1s = 4,
    on125msPer250ms = 5,
    strobe = 6
}
--- @enum DAQ_SignalingBuzzerPatternEnum
DAQ.SignalingBuzzerPatternEnum = {
    off = 0,
    on100msPer3s = 1,
    on500msPer1s = 2,
    on900msPer1s = 3,
    on100msPer1s = 4,
    on125msPer250ms = 5
}
--- @enum DAQ_SignalingColors
DAQ.SignalingColors = {
    black = 0xE0000000,
    red = 0xFF0000FF,
    green = 0xFF00FF00,
    blue = 0xFFFF0000,
    yellow = 0xFF0039FF, -- Closer to orange than yellow.
    cyan = 0xFFFFFF00,
    magenta = 0xFFFF00FF,
    white = 0xFFFFFFFF
}
--- @enum DAQ_SignalingLed
DAQ.SignalingLed = {
    led1 = "LED 1 Color",
    led2 = "LED 2 Color",
    led3 = "LED 3 Color",
    led4 = "LED 4 Color",
    led5 = "LED 5 Color",
    led6 = "LED 6 Color",
    led7 = "LED 7 Color",
    led8 = "LED 8 Color",
}

function DAQ.SignalingModeEnumToString(mode)
    local names = {
        [DAQ.SignalingModeEnum.off] = "off",
        [DAQ.SignalingModeEnum.idle] = "idle",
        [DAQ.SignalingModeEnum.standby] = "standby",
        [DAQ.SignalingModeEnum.busy] = "busy",
        [DAQ.SignalingModeEnum.concern] = "concern",
        [DAQ.SignalingModeEnum.lockOut] = "lockOut",
        [DAQ.SignalingModeEnum.danger] = "danger",
        [DAQ.SignalingModeEnum.mutedDanger] = "mutedDanger",
        [DAQ.SignalingModeEnum.boot] = "boot",
        [DAQ.SignalingModeEnum.custom] = "custom"
    }
    local name = names[mode]
    if name == nil then name = "unknown" end
    return name;
end

--- Gets or set the signaling mode.
--- @param mode? DAQ_SignalingModeEnum
--- @return DAQ_SignalingModeEnum?
function DAQ:SignalingMode(mode)
    local od = self.ib.od["Signaling"]["Mode"]
    if mode == nil then
        return self.ib:Upload(od) --[[@as DAQ_SignalingModeEnum]]
    else
        CheckField(mode, "mode", IsIntegerIn(mode, 0, 9))
        self.ib:Download(od, mode)
    end
end

--- Gets or set a signaling mode modifier.
--- @param mod? DAQ_SignalingModeModifierEnum
--- @return DAQ_SignalingModeModifierEnum?
function DAQ:SignalingModeModifier(mod)
    local od = self.ib.od["Signaling"]["Modifier"]
    if mod == nil then
        return self.ib:Upload(od) --[[@as DAQ_SignalingModeEnum]]
    else
        CheckField(mod, "mode", IsIntegerIn(mod, 0, 6))
        self.ib:Download(od, mod)
    end
end

--- Gets or set the idle time.
--- @param minutes? integer
--- @return integer?
function DAQ:SignalingIdleTime(minutes)
    local od = self.ib.od["Signaling"]["Idle Time"]
    if minutes == nil then
        return self.ib:Upload(od) --[[@as integer]]
    else
        CheckField(minutes, "idle time", IsIntegerInOd(minutes, od))
        self.ib:Download(od, minutes)
    end
end

--- Gets or set the color of a LED.
--- @param led DAQ_SignalingLed
--- @param color DAQ_SignalingColors|integer|nil
--- @return integer?
function DAQ:SignalingLedColor(led, color)
    local od = self.ib.od["Signaling"][led]
    CheckField(led, "led", od ~= nil)
    if color == nil then
        return self.ib:Upload(od) --[[@as integer]]
    else
        CheckField(color, "Color", IsIntegerInOd(color, od))
        self.ib:Download(od, color)
    end
end

--- @class SignalingBuzzerModeReturn
--- @field pattern DAQ_SignalingBuzzerPatternEnum
--- @field duration integer

--- Gets or set the signaling buzzer mode.
--- @param pattern? DAQ_SignalingBuzzerPatternEnum
--- @param duration? integer
--- @return SignalingBuzzerModeReturn?
function DAQ:SignalingBuzzerMode(pattern, duration)
    local od = self.ib.od["Signaling"]["Buzzer Mode"]
    if pattern == nil and duration == nil then
        local result = self.ib:Upload(od)
        return { pattern = result & 0xFF, duration = (result >> 8) & 0xFF }
    elseif pattern == nil and duration ~= nil then
        error("Invalid pattern, nil")
    else
        if duration == nil then duration = 255 end
        CheckField(pattern, "pattern", IsIntegerIn(pattern, DAQ.SignalingBuzzerPatternEnum.off,
            DAQ.SignalingBuzzerPatternEnum.on125msPer250ms))
        CheckField(duration, "duration", IsIntegerIn(duration, 0, 255))
        self.ib:Download(od, pattern | (duration << 8))
    end
end

-- UUT Ground
--- Gets or sets the state of the UUT ground.
--- @param state? boolean
--- @return boolean?
function DAQ:UutGround(state)
    local od = self.ib.od["UUT Ground"]
    if state == nil then
        return self.ib:Upload(od) --[[@as boolean]]
    else
        CheckField(state, "state", IsBoolean(state))
        self.ib:Download(od, state)
    end
end

-- Internal Can
--- Gets or set the state of the internal CAN's standby signal.
--- @param state? boolean
--- @return boolean?
function DAQ:InternalCanStandby(state)
    local od = self.ib.od["Internal CAN"]["Standby"]
    if state == nil then
        return self.ib:Upload(od) --[[@as boolean]]
    else
        CheckField(state, "state", IsBoolean(state))
        self.ib:Download(od, state)
    end
end

--- Gets or sets the state of the internal CAN's termincation resistor.
--- @param state? boolean
--- @return boolean?
function DAQ:InternalCanTerminationResistor(state)
    local od = self.ib.od["Internal CAN"]["Termination Resistor"]
    if state == nil then
        return self.ib:Upload(od) --[[@as boolean]]
    else
        CheckField(state, "state", IsBoolean(state))
        self.ib:Download(od, state)
    end
end

-- I2C
--- @enum DAQ_I2CGpioEnum
DAQ.I2CGpioEnum = {}

-- SPI
--- @enum DAQ_SpiGpioEnum
DAQ.SpiGpioEnum = { mosi = 0, miso = 1, sck = 2, }
--- @enum DAQ_SpiModeEnum
DAQ.SpiModeEnum = { gpio = 0, spi = 1, }

--- Property for SPI mode
--- If mode is nil, act as a Setter, Getter otherwise
--- @param mode DAQ_SpiModeEnum?
--- @return DAQ_SpiModeEnum? mode
function DAQ:SpiMode(mode)
    --- TODO
end

--- Property for SPI GPIO Configuration
--- Valid only when SPI is set into gpio mode
--- If value is nil, act as a Setter, Getter otherwise
--- @param value number?
--- @return number? state
function DAQ:SpiGpioConfigurations(value)
    --- TODO
end

--- Property for SPI GPIO Values
--- Valid only when SPI is set into gpio mode
--- If value is nil, act as a Setter, Getter otherwise
--- @param value number?
--- @return number? state
function DAQ:SpiGpioValues(value)
    --- TODO
end

--- Property for SPI GPIO Values
--- Valid only when SPI is set into gpio mode
--- If value is nil, act as a Setter, Getter otherwise
--- @param value number?
--- @return number? state
function DAQ:SpiGpioPolarities(value)
end

-- ADC
--- @enum DAQ_AdcChannelEnum
DAQ.AdcChannelEnum = { adc1 = 1, adc2 = 2, adc3 = 3, adc4 = 4 }
--- @enum DAQ_AdcSampleRateEnum
DAQ.AdcSampleRateEnum = {
    f250Hz = 0,
    f500Hz = 1,
    f1000Hz = 2,
    f2000Hz = 3,
    f2560Hz = 4,
    f2667Hz = 5,
    f4000Hz = 6,
    f5120Hz = 7,
    f5333Hz = 8,
    f8000Hz = 9,
    f10240Hz = 10,
    f10667Hz = 11,
    f16000Hz = 12,
    f20480Hz = 13,
    f21333Hz = 14,
    f32000Hz = 15,
    f42667Hz = 16,
    f64000Hz = 17,
    f85333Hz = 18,
    f128000Hz = 19
}
DAQ.AdcSampleRateValues = {
    [DAQ.AdcSampleRateEnum.f250Hz] = 250,
    [DAQ.AdcSampleRateEnum.f500Hz] = 500,
    [DAQ.AdcSampleRateEnum.f1000Hz] = 1000,
    [DAQ.AdcSampleRateEnum.f2000Hz] = 2000,
    [DAQ.AdcSampleRateEnum.f2560Hz] = 2560,
    [DAQ.AdcSampleRateEnum.f2667Hz] = 2667,
    [DAQ.AdcSampleRateEnum.f4000Hz] = 4000,
    [DAQ.AdcSampleRateEnum.f5120Hz] = 5120,
    [DAQ.AdcSampleRateEnum.f5333Hz] = 5333,
    [DAQ.AdcSampleRateEnum.f8000Hz] = 8000,
    [DAQ.AdcSampleRateEnum.f10240Hz] = 10240,
    [DAQ.AdcSampleRateEnum.f10667Hz] = 10667,
    [DAQ.AdcSampleRateEnum.f16000Hz] = 16000,
    [DAQ.AdcSampleRateEnum.f20480Hz] = 20480,
    [DAQ.AdcSampleRateEnum.f21333Hz] = 21333,
    [DAQ.AdcSampleRateEnum.f32000Hz] = 32000,
    [DAQ.AdcSampleRateEnum.f42667Hz] = 42667,
    [DAQ.AdcSampleRateEnum.f64000Hz] = 64000,
    [DAQ.AdcSampleRateEnum.f85333Hz] = 85333,
    [DAQ.AdcSampleRateEnum.f128000Hz] = 128000
}
--- @enum DAQ_AdcChannelGainEnum
DAQ.AdcChannelGainEnum = { g1 = 0, g2 = 1, g4 = 2, g8 = 3, g16 = 4 }
--- Translates a channel to its corresponding integer.
--- @param channel DAQ_AdcChannelEnum
--- @return integer
local function AdcChannelToInt(channel)
    local indexes = {
        [DAQ.AdcChannelEnum.adc1] = 1,
        [DAQ.AdcChannelEnum.adc2] = 2,
        [DAQ.AdcChannelEnum.adc3] = 3,
        [DAQ.AdcChannelEnum.adc4] = 4
    }
    return indexes[channel]
end

--- Translates a channel to its corresponding test point.
--- @param channel DAQ_AdcChannelEnum
--- @return DAQ_RoutingPointsEnum|nil
function DAQ.AdcChannelToTestPoint(channel)
    local indexes = {
        [DAQ.AdcChannelEnum.adc1] = nil,
        [DAQ.AdcChannelEnum.adc2] = DAQ.RoutingPointsEnum.ADC_CH1,
        [DAQ.AdcChannelEnum.adc3] = DAQ.RoutingPointsEnum.ADC_CH2,
        [DAQ.AdcChannelEnum.adc4] = nil
    }
    return indexes[channel]
end

local function CheckAdcChannel(channel)
    CheckField(channel, "channel", IsIntegerIn(channel, DAQ.AdcChannelEnum.adc1, DAQ.AdcChannelEnum.adc4))
end

--- @private
--- Gets the object dictionary entry of a channel.
--- @param channel DAQ_AdcChannelEnum
--- @param affix string
--- @return any
function DAQ:GetAdcChannelOb(channel, affix)
    return self.ib.od["ADC"]["Channel " .. AdcChannelToInt(channel) .. " " .. affix]
end

--- Gets the value read by the ID Check ADC channel.
--- @return number
function DAQ:AdcIdCheck() return self.ib:Upload(self.ib.od["ADC"]["ID Check"]) --[[@as number]] end

--- Gets or sets the number of samples to take on the ADC.
--- @param count integer?
--- @return integer?
function DAQ:AdcSamplesToTake(count)
    local od = self.ib.od["ADC"]["Samples to Take"]
    if count == nil then
        return self.ib:Upload(od) --[[@as integer]]
    else
        CheckField(count, "count", IsIntegerInOd(count, od))
        self.ib:Download(od, count)
    end
end

--- Gets or sets the sample rate of the ADC.
--- @param sampleRate DAQ_AdcSampleRateEnum
--- @return DAQ_AdcSampleRateEnum?
function DAQ:AdcSampleRate(sampleRate)
    local od = self.ib.od["ADC"]["Sample Rate"]
    if sampleRate == nil then
        return self.ib:Upload(od) --[[@as DAQ_AdcSampleRateEnum]]
    else
        CheckField(sampleRate, "sample rate", IsIntegerInOd(sampleRate, od))
        self.ib:Download(od, sampleRate)
    end
end

--- Gets or sets the gain on a channel of the ADC.
--- @param channel DAQ_AdcChannelEnum
--- @param gain DAQ_AdcChannelGainEnum?
--- @return DAQ_AdcChannelGainEnum?
function DAQ:AdcChannelGain(channel, gain)
    CheckAdcChannel(channel)
    local od = self:GetAdcChannelOb(channel, "Gain")
    if gain == nil then
        return self.ib:Upload(od) --[[@as DAQ_AdcChannelGainEnum]]
    else
        CheckField(gain, "gain", IsIntegerInOd(gain, od))
        self.ib:Download(od, gain)
    end
end

--- @class DAQ_AdcChannelResults
--- @field min number
--- @field average number
--- @field max number

--- Gets the results for one of the channels of the ADC.
--- @param channel DAQ_AdcChannelEnum
--- @return DAQ_AdcChannelResults
function DAQ:AdcChannelResults(channel)
    CheckAdcChannel(channel)
    local odMin = self:GetAdcChannelOb(channel, "Min")
    local odMax = self:GetAdcChannelOb(channel, "Max")
    local odAvg = self:GetAdcChannelOb(channel, "Average")

    return {
        min = self.ib:Upload(odMin) --[[@as number]],
        max = self.ib:Upload(odMax) --[[@as number]],
        average = self.ib:Upload(odAvg) --[[@as number]]
    }
end

--- Gets the average value read on a channel.
--- @param channel DAQ_AdcChannelEnum
--- @return number
function DAQ:AdcChannelAverage(channel)
    CheckAdcChannel(channel)
    local od = self:GetAdcChannelOb(channel, "Average")
    return self.ib:Upload(od) --[[@as number]]
end

--- @param index integer
function DAQ:AdcStoredSampleIndex(index)
    CheckField(index, "index", IsIntegerIn(index, 0, 999))
    local od = self.ib.od["ADC"]["Stored Sample Index"]
    self.ib:Download(od, index)
end

--- @param channel DAQ_AdcChannelEnum
--- @return number
function DAQ:AdcStoredSampleValue(channel)
    local od
    if channel == DAQ.AdcChannelEnum.adc2 then
        od = self.ib.od["ADC"]["Stored Sample Channel 2"]
    elseif channel == DAQ.AdcChannelEnum.adc3 then
        od = self.ib.od["ADC"]["Stored Sample Channel 3"]
    else
        error("Invalid channel: " .. ToString(channel))
    end
    return self.ib:Upload(od) --[[@as number]]
end

--- @class AdcComplexCalibration
--- @field gain {r100: number, r4k99: number, r100k: number, r1M: number}
--- @field offset {r100: number, r4k99: number, r100k: number, r1M: number}

--- @class AdcMoreComplexCalibration
--- @field gain {base: number, r100: number, r4k99: number, r100k: number, r1M: number}
--- @field offset {base: number, r100: number, r4k99: number, r100k: number, r1M: number}

-- ADC Calibration
--- Gets the calibration table.
--- @return [AdcComplexCalibration, AdcMoreComplexCalibration, {gain: number, offset: number}]
function DAQ:AdcCalibration()
    local od = self.ib.od["ADC Calibration"]
    --- @return number
    local function f(od) return self.ib:Upload(od) --[[@as number]] end
    return {
        [1] = {
            gain = {
                r100 = f(od["Channel 1 Gain 100R"]),
                r4k99 = f(od["Channel 1 Gain 4k99"]),
                r100k = f(od["Channel 1 Gain 100k"]),
                r1M = f(od["Channel 1 Gain 1M"])
            },
            offset = {
                r100 = f(od["Channel 1 Offset 100R"]),
                r4k99 = f(od["Channel 1 Offset 4k99"]),
                r100k = f(od["Channel 1 Offset 100k"]),
                r1M = f(od["Channel 1 Offset 1M"])
            }
        },
        [2] = {
            gain = {
                base = f(od["Channel 2 Gain"]),
                r100 = f(od["Channel 2 Gain 100R"]),
                r4k99 = f(od["Channel 2 Gain 4k99"]),
                r100k = f(od["Channel 2 Gain 100k"]),
                r1M = f(od["Channel 2 Gain 1M"])
            },
            offset = {
                base = f(od["Channel 2 Offset"]),
                r100 = f(od["Channel 2 Offset 100R"]),
                r4k99 = f(od["Channel 2 Offset 4k99"]),
                r100k = f(od["Channel 2 Offset 100k"]),
                r1M = f(od["Channel 2 Offset 1M"])
            }
        },
        [3] = { gain = f(od["Channel 3 Gain"]), offset = f(od["Channel 3 Offset"]) }
    }
end

-- Impedances
--- @enum DAQ_ImpedanceModeEnum
DAQ.ImpedanceModeEnum = { resistor = 0, capacitor = 1, inductor = 2 }
--- @enum DAQ_ImpedanceShapeEnum
DAQ.ImpedanceShapeEnum = { automatic = 0, dc = 1, sine = 2, triangle = 3, ramp = 4, noise = 5 }
--- @enum DAQ_ImpedanceRangeResistorEnum
DAQ.ImpedanceRangeResistorEnum = { automatic = 0, r100 = 1, r4k99 = 2, r100k = 3, r1M = 4 }
--- @class DAQ_ImpedanceDefaults
--- @field shape DAQ_ImpedanceShapeEnum
--- @field rangeResistor DAQ_ImpedanceRangeResistorEnum
DAQ.ImpedanceDefaults = {
    shape = DAQ.ImpedanceShapeEnum.automatic,
    rangeResistor = DAQ.ImpedanceRangeResistorEnum.automatic,
    frequency = 0,
    amplitude = 0,
    delay = 0,
    samplesToTake = 0,
    expectedValue = 0,
    favorSpeed = false
}

---@param samplesToTake integer
---@param sampleRate DAQ_AdcSampleRateEnum
---@param delay integer? delay in us
---@return number duration in s
local function computeMeasureDuration(samplesToTake, sampleRate, delay)
    local delayDuration = delay / 1000 / 1000
    local measureDuration = samplesToTake / DAQ.AdcSampleRateValues[sampleRate]
    local commDuration = 0.5 -- 500ms extra duration
    return delayDuration + measureDuration + commDuration
end

--- @class DAQ_ImpedancesOptionalParameters
--- @field shape DAQ_ImpedanceShapeEnum?
--- @field rangeResistor DAQ_ImpedanceRangeResistorEnum?
--- @field frequency integer?
--- @field amplitude integer?
--- @field expectedValue integer? Expected value to be read. Required if range if set to automatic
--- @field range DAQ_ImpedanceRangeResistorEnum?
--- @field guards DAQ_RoutingPointsEnum[]?
--- @field voltage number?
--- @field delay integer?
--- @field samplesToTake integer?
--- @field favorSpeed boolean?

--- @class DAQ_ImpedanceResults
--- @field mode DAQ_ImpedanceModeEnum
--- @field shape DAQ_ImpedanceShapeEnum
--- @field rangeResistor DAQ_ImpedanceRangeResistorEnum
--- @field frequency integer
--- @field amplitude integer
--- @field delay integer
--- @field samplesToTake integer
--- @field expectedValue number
--- @field favorSpeed boolean
--- @field confidence integer
--- @field value number
--- @field vin number
--- @field vout number

--- Measures an impedance.
--- @param mode DAQ_ImpedanceModeEnum
--- @param opt DAQ_ImpedancesOptionalParameters?
--- @return DAQ_ImpedanceResults
function DAQ:Impedances(mode, opt)
    local od = self.ib.od["Impedances"]
    local odTrigger = od["Trigger"]
    if self.ib:Upload(odTrigger) then error("Impedance measure already running") end

    local odMode = od["Mode"]
    CheckField(mode, "mode", IsIntegerInOd(mode, odMode))
    local odShape = od["Shape"]
    local odRangeResistor = od["Range Resistor"]
    local odFrequency = od["Frequency"]
    local odAmplitude = od["Amplitude"]
    local odDelay = od["Delay"]
    local odSamplesToTake = od["Samples to Take"]
    local odExpectedValue = od["Expected Value"]
    local odFavorSpeed = od["Favor Speed"]
    local odConfidence = od["Confidence"]
    local odValue = od["Value"]
    local odVin = od["Vin"]
    local odVout = od["Vout"]

    if opt == nil then opt = {} end
    CheckField(opt, "opt", type(opt) == "table")

    if opt.shape == nil then opt.shape = DAQ.ImpedanceDefaults.shape end
    if opt.rangeResistor == nil then opt.rangeResistor = DAQ.ImpedanceDefaults.rangeResistor end
    if opt.frequency == nil then opt.frequency = DAQ.ImpedanceDefaults.frequency end
    if opt.amplitude == nil then opt.amplitude = DAQ.ImpedanceDefaults.amplitude end
    if opt.delay == nil then opt.delay = DAQ.ImpedanceDefaults.delay end
    if opt.samplesToTake == nil then opt.samplesToTake = DAQ.ImpedanceDefaults.samplesToTake end
    if opt.expectedValue == nil then opt.expectedValue = DAQ.ImpedanceDefaults.expectedValue end
    if opt.favorSpeed == nil then opt.favorSpeed = DAQ.ImpedanceDefaults.favorSpeed end

    CheckField(opt.shape, "shape", IsIntegerInOd(opt.shape, odShape))
    CheckField(opt.rangeResistor, "range Resistor", IsIntegerInOd(opt.rangeResistor, odRangeResistor))
    CheckField(opt.frequency, "frequency", IsIntegerInOd(opt.frequency, odFrequency))
    CheckField(opt.amplitude, "amplitude", IsFloatInOd(opt.amplitude, odAmplitude))
    CheckField(opt.samplesToTake, "samples to take", IsIntegerInOd(opt.samplesToTake, odSamplesToTake))
    CheckField(opt.delay, "delay", IsIntegerIn(opt.delay, 0, 4294967296))
    CheckField(opt.expectedValue, "expected value", IsIntegerIn(opt.expectedValue, 0, 4294967296))
    CheckField(opt.favorSpeed, "favorSpeed", IsBoolean(opt.favorSpeed))


    -- check auto parameters
    -- If resistor, expectedValue cannot be 0 if rangeResitor is automatic
    -- If capacitor or inductor, expectedValue cannot be 0 if frequency or rangeResistor are automatic
    if (mode == DAQ.ImpedanceModeEnum.resistor and opt.expectedValue == 0 and opt.rangeResistor == 0)
        or ((mode == DAQ.ImpedanceModeEnum.capacitor or mode == DAQ.ImpedanceModeEnum.inductor)
            and opt.expectedValue == 0 and (opt.frequency == 0 or opt.rangeResistor == 0))
    then
        error("Expected Value cannot be set to automatic when Range Resistor or Frequency are as well")
    end

    self.ib:Download(odMode, mode)
    self.ib:Download(odShape, opt.shape)
    self.ib:Download(odRangeResistor, opt.rangeResistor)
    self.ib:Download(odFrequency, opt.frequency)
    self.ib:Download(odAmplitude, opt.amplitude)
    self.ib:Download(odDelay, opt.delay)
    self.ib:Download(odSamplesToTake, opt.samplesToTake)
    self.ib:Download(odExpectedValue, opt.expectedValue)
    self.ib:Download(odFavorSpeed, opt.favorSpeed)
    self.ib:Download(odTrigger, true)

    if Context.info.stage == Stage.execution then
        SleepFor(100) -- Let DAQ start measure and update trigger
        local deadline = os.clock() + computeMeasureDuration(opt.samplesToTake, DAQ.AdcSampleRateEnum.f1000Hz, opt.delay)
        local doRun = true
        while doRun do
            doRun = self.ib:Upload(odTrigger) --[[@as boolean]]
            SleepFor(10)
            if deadline < os.clock() then error("Impedance timeout") end
        end
    end

    if opt.shape == 0 then opt.shape = self.ib:Upload(odShape) --[[@as DAQ_ImpedanceShapeEnum]] end
    if opt.rangeResistor == 0 then opt.rangeResistor = self.ib:Upload(odRangeResistor) --[[@as DAQ_ImpedanceRangeResistorEnum]] end
    if opt.frequency == 0 then opt.frequency = self.ib:Upload(odFrequency) --[[@as integer]] end
    if opt.amplitude == 0 then opt.amplitude = self.ib:Upload(odAmplitude) --[[@as number]] end
    if opt.delay == 0 then opt.delay = self.ib:Upload(odDelay) --[[@as integer]] end
    if opt.samplesToTake == 0 then opt.samplesToTake = self.ib:Upload(odSamplesToTake) --[[@as integer]] end
    if opt.expectedValue == 0 then opt.expectedValue = self.ib:Upload(odExpectedValue) --[[@as integer]] end

    return {
        mode = mode,
        shape = opt.shape --[[@as DAQ_ImpedanceShapeEnum]],
        rangeResistor = opt.rangeResistor --[[@as DAQ_ImpedanceRangeResistorEnum]],
        frequency = opt.frequency --[[@as integer]],
        amplitude = opt.amplitude --[[@as number]],
        delay = opt.delay --[[@as integer]],
        samplesToTake = opt.samplesToTake --[[@as integer]],
        expectedValue = opt.expectedValue --[[@as number]],
        favorSpeed = opt.favorSpeed,
        confidence = self.ib:Upload(odConfidence) --[[@as integer]],
        value = self.ib:Upload(odValue) --[[@as number]],
        vin = self.ib:Upload(odVin) --[[@as number]],
        vout = self.ib:Upload(odVout) --[[@as number]]
    }
end

-- High level functions
--- @param point DAQ_RoutingPointsEnum|DAQ_RoutingPointsEnum[]
--- @return DAQ_RoutingPointsEnum[]
local function PointToPoints(point)
    if type(point) ~= "table" then return { point } end
    return point --[[@as DAQ_RoutingPointsEnum[] ]]
end

local function IsPointsOk(points)
    if type(points) ~= table and #points == 0 then return false end
    for _, v in ipairs(points) do if not IsIntegerIn(points, 0, 75) then return false end end
    return true
end

DAQ.MeasureVoltageDefault = {
    channel = DAQ.AdcChannelEnum.adc2 --[[@as DAQ_AdcChannelEnum]],
    samplesToTake = 10,
    gain = DAQ.AdcChannelGainEnum.g1 --[[@as DAQ_AdcChannelGainEnum]],
    sampleRate = DAQ.AdcSampleRateEnum.f1000Hz --[[@as DAQ_AdcSampleRateEnum]]
}

--- @class DAQ_MeasureVoltageOptParameters
--- @field channel DAQ_AdcChannelEnum? which ADC to do the measure on
--- @field samplesToTake integer? number of samples to take [1, 1000]
--- @field gain DAQ_AdcChannelGainEnum? ADC gain
--- @field sampleRate DAQ_AdcSampleRateEnum? ADC sampling rate

--- Measures a voltage on one or more points.
--- @param points DAQ_RoutingPointsEnum[]|DAQ_RoutingPointsEnum place where to measure voltage
--- @param opt DAQ_MeasureVoltageOptParameters?
--- @return DAQ_AdcChannelResults
function DAQ:MeasureVoltage(points, opt)
    --- @type DAQ_RoutingPointsEnum[]
    points = PointToPoints(points)
    if IsPointsOk(points) then error("Invalid points") end
    if opt == nil then opt = {} end
    CheckField(opt, "opt", type(opt) == "table")
    if opt.channel == nil then opt.channel = DAQ.MeasureVoltageDefault.channel end
    CheckField(opt.channel, "Channel", opt.channel == DAQ.AdcChannelEnum.adc2 or opt.channel == DAQ.AdcChannelEnum.adc3)
    if opt.samplesToTake == nil then opt.samplesToTake = DAQ.MeasureVoltageDefault.samplesToTake end
    if opt.gain == nil then opt.gain = DAQ.MeasureVoltageDefault.gain end
    if opt.sampleRate == nil then opt.sampleRate = DAQ.MeasureVoltageDefault.sampleRate end

    local route = self:RequestRouting({ table.unpack(points), DAQ.AdcChannelToTestPoint(opt.channel) })

    if route == -1 then
        error("Unable to connect points to ADC!")
    end

    self:AdcChannelGain(opt.channel, opt.gain)
    self:AdcSampleRate(opt.sampleRate)
    self:AdcSamplesToTake(opt.samplesToTake)

    while opt.samplesToTake ~= 0 do
        local previous = opt.samplesToTake
        TimeoutFunction(function()
            local current = self:AdcSamplesToTake() --[[@as integer]]
            local same = current == previous
            previous = current
            return same
        end, 1000)
        opt.samplesToTake = previous
    end

    self:ClearBus(route)

    return self:AdcChannelResults(opt.channel)
end

---@param channel DAQ_AdcChannelEnum
---@param index integer
---@return number
function DAQ:GetStoredSample(channel, index)
    self:AdcStoredSampleIndex(index)
    return self:AdcStoredSampleValue(channel)
end

--- @class DAQ_MeasureResistorOptParameters
--- @field expectedValue integer? Expected value to be read. Required if range if set to automatic
--- @field rangeResistor DAQ_ImpedanceRangeResistorEnum?
--- @field guards DAQ_RoutingPointsEnum[]?
--- @field amplitude number?
--- @field delay integer? us
--- @field samplesToTake integer?
--- @field favorSpeed boolean?

--- Measures a resistor.
--- @param impP DAQ_RoutingPointsEnum|DAQ_RoutingPointsEnum[]
--- @param impN DAQ_RoutingPointsEnum|DAQ_RoutingPointsEnum[]
--- @param opt DAQ_MeasureResistorOptParameters?
--- @return DAQ_ImpedanceResults
function DAQ:MeasureResistor(impP, impN, opt)
    if opt == nil then opt = {} end
    CheckField(opt, "opt", type(opt) == "table")

    impP = PointToPoints(impP)
    impN = PointToPoints(impN)

    local rimpp = self:RequestRouting({ table.unpack(impP), DAQ.RoutingPointsEnum.IMP_P, DAQ.RoutingPointsEnum.ADC_CH1 })
    SleepFor(10)
    local rimpn = self:RequestRouting({ table.unpack(impN), DAQ.RoutingPointsEnum.IMP_N })
    local rguards = nil
    if opt.guards ~= nil then
        opt.guards = PointToPoints(opt.guards)
        rguards = self:RequestRouting({ table.unpack(opt.guards), DAQ.RoutingPointsEnum.GUARD })
    end

    local result = self:Impedances(
        DAQ.ImpedanceModeEnum.resistor, {
            shape = DAQ.ImpedanceShapeEnum.dc,
            rangeResistor = opt.rangeResistor,
            amplitude = opt.amplitude,
            delay = opt.delay,
            samplesToTake = opt.samplesToTake,
            expectedValue = opt.expectedValue,
            favorSpeed = opt.favorSpeed
        })

    -- cleanup
    self:ClearBus(rimpp)
    self:ClearBus(rimpn)
    if rguards ~= nil then self:ClearBus(rguards) end

    return result
end

--- @class DAQ_MeasureCapacitorOptParameters
--- @field frequency number?
--- @field expectedValue integer? Expected value to be read. Required if range if set to automatic
--- @field rangeResistor DAQ_ImpedanceRangeResistorEnum?
--- @field guards DAQ_RoutingPointsEnum[]?
--- @field amplitude number?
--- @field delay integer?
--- @field samplesToTake integer?
--- @field favorSpeed boolean?

--- Measures a capacitor.
--- @param impP DAQ_RoutingPointsEnum|DAQ_RoutingPointsEnum[]
--- @param impN DAQ_RoutingPointsEnum|DAQ_RoutingPointsEnum[]
--- @param opt DAQ_MeasureCapacitorOptParameters?
--- @return DAQ_ImpedanceResults
function DAQ:MeasureCapacitor(impP, impN, opt)
    if opt == nil then opt = {} end
    CheckField(opt, "opt", type(opt) == "table")

    impP = PointToPoints(impP)
    impN = PointToPoints(impN)

    local rimpp = self:RequestRouting({ table.unpack(impP), DAQ.RoutingPointsEnum.IMP_P, DAQ.RoutingPointsEnum.ADC_CH1 })
    SleepFor(10)
    local rimpn = self:RequestRouting({ table.unpack(impN), DAQ.RoutingPointsEnum.IMP_N })
    local rguards = nil
    if opt.guards ~= nil then
        opt.guards = PointToPoints(opt.guards)
        rguards = self:RequestRouting({ table.unpack(opt.guards), DAQ.RoutingPointsEnum.GUARD })
    end

    local result = self:Impedances(
        DAQ.ImpedanceModeEnum.capacitor, {
            frequency = opt.frequency,
            rangeResistor = opt.rangeResistor,
            amplitude = opt.amplitude,
            delay = opt.delay,
            samplesToTake = opt.samplesToTake,
            expectedValue = opt.expectedValue,
            favorSpeed = opt.favorSpeed
        })

    -- cleanup
    self:ClearBus(rimpp)
    self:ClearBus(rimpn)
    if rguards ~= nil then self:ClearBus(rguards) end

    return result
end

--- @class DAQ_MeasureInductorOptParameters
--- @field frequency number?
--- @field expectedValue integer? Expected value to be read. Required if range if set to automatic
--- @field rangeResistor DAQ_ImpedanceRangeResistorEnum?
--- @field guards DAQ_RoutingPointsEnum[]?
--- @field amplitude number?
--- @field delay integer?
--- @field samplesToTake integer?
--- @field favorSpeed boolean?

--- Measures an inductor.
--- @param impP DAQ_RoutingPointsEnum|DAQ_RoutingPointsEnum[]
--- @param impN DAQ_RoutingPointsEnum|DAQ_RoutingPointsEnum[]
--- @param opt DAQ_MeasureInductorOptParameters?
--- @return DAQ_ImpedanceResults
function DAQ:MeasureInductor(impP, impN, opt)
    if opt == nil then opt = {} end
    CheckField(opt, "opt", type(opt) == "table")

    impP = PointToPoints(impP)
    impN = PointToPoints(impN)

    local rimpp = self:RequestRouting({ table.unpack(impP), DAQ.RoutingPointsEnum.IMP_P, DAQ.RoutingPointsEnum.ADC_CH1 })
    SleepFor(10)
    local rimpn = self:RequestRouting({ table.unpack(impN), DAQ.RoutingPointsEnum.IMP_N })
    local rguards = nil
    if opt.guards ~= nil then
        opt.guards = PointToPoints(opt.guards)
        rguards = self:RequestRouting({ table.unpack(opt.guards), DAQ.RoutingPointsEnum.GUARD })
    end

    local result = self:Impedances(
        DAQ.ImpedanceModeEnum.inductor, {
            frequency = opt.frequency,
            rangeResistor = opt.rangeResistor,
            amplitude = opt.amplitude,
            delay = opt.delay,
            samplesToTake = opt.samplesToTake,
            expectedValue = opt.expectedValue,
            favorSpeed = opt.favorSpeed
        })

    -- cleanup
    self:ClearBus(rimpp)
    self:ClearBus(rimpn)
    if rguards ~= nil then self:ClearBus(rguards) end

    return result
end

function DAQ:LogCodeDec(code)
    self.ib:Download(self.ib.od["LogCode"]["Dec"], code)
end

function DAQ:LogCodeHex(code)
    self.ib:Download(self.ib.od["LogCode"]["Hex"], code)
end
