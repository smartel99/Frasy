--- @type Ib
local Ib = require("lua/core/sdk/environment/ib")
local Bitwise = require("lua/core/utils/bitwise")
local IsBoolean = require("lua/core/utils/is_boolean")
local IsInteger = require("lua/core/utils/is_integer/is_integer")
local IsIntegerIn = require("lua/core/utils/is_integer/is_integer_in")
local IsIntegerInOd = require("lua/core/utils/is_integer/is_integer_in_od")
local IsUnsigned = require("lua/core/utils/is_unsigned/is_unsigned")
local IsUnsignedIn = require("lua/core/utils/is_unsigned/is_unsigned_in")
local IsUnsignedInOd = require("lua/core/utils/is_unsigned/is_unsigned_in_od")
local IsFloat = require("lua/core/utils/is_float/is_float")
local IsFloatIn = require("lua/core/utils/is_float/is_float_in")
local IsFloatInOd = require("lua/core/utils/is_float/is_float_in_od")
local TimeoutFunction = require("lua/core/utils/timeout_function")
local CheckField = require("lua/core/utils/check_field")
local StringizeValues = require("lua/core/utils/stringize_values")
local TryFunction = require("lua/core/utils/try_function")

--- @class DAQ_CacheIo
--- @field mode DAQ_IoModeEnum
--- @field value DAQ_IoValueEnum

--- @class DAQ_Cache
--- @field io DAQ_CacheIo

--- @class DAQ
--- @field ib? Ib
--- @field cache DAQ_Cache
DAQ = { ib = nil, cache = { io = { mode = 0, value = 0 } } }
DAQ.__index = DAQ

--- Instantiates a DAQ card.
--- @param name? string Name of the card
--- @param nodeId? integer ID of the Node
--- @return DAQ
function DAQ:New(name, nodeId)
    local ib = Ib:New()
    ib.kind = 02;
    if name == nil then name = "daq" end
    ib.name = name
    if nodeId == nil then nodeId = ib.kind end
    ib.nodeId = nodeId
    ib.eds = "lua/core/cep/eds/daq_1.0.0.eds"
    return setmetatable({ ib = ib, cache = { io = { mode = 0, value = 0 } } }, DAQ)
end

function DAQ:LoadCache()
    self:IoModes()
    self:IoValues()
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
    GUARD = 75
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
        [DAQ.RoutingPointsEnum.GUARD] = "GUARD"
    }
    return names[point]
end

local function _requestRouting(daq, points)
    daq.ib:Download(daq.ib.od["Routing"]["Request Routing"], points)
    Utils.SleepFor(1)
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
DAQ.DacShapeEnum = { dc = 0, sine = 1, sawtooth = 2, triangle = 3, square = 4, noise = 5 }

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
--- @enum DAQ_IoEnum
DAQ.IoEnum = { a = 0, b = 1, spiUutSck = 2, spiUutMosi = 3, spiUutMiso = 4, i2cUutScl = 5, i2cUutSda = 6, uartUutTx = 7, uartUutRx = 8 }
--- @enum DAQ_IoModeEnum
DAQ.IoModeEnum = { input = 0, output = 1 }
--- @enum DAQ_IoValueEnum
DAQ.IoValueEnum = { low = 0, high = 1 }

local function CheckIo(io) CheckField(io, "io", IsIntegerIn(io, DAQ.IoEnum.a, DAQ.IoEnum.uartUutRx)) end

--- Gets or set the mode of the IOs.
--- @param value? DAQ_IoModeEnum
--- @return DAQ_IoModeEnum?
function DAQ:IoModes(value)
    local od = self.ib.od["IO"]["Mode"]
    if (value == nil) then
        self.cache.io.mode = self.ib:Upload(od) --[[@as DAQ_IoModeEnum]]
        return self.cache.io.mode
    else
        self.cache.io.mode = value
        self.ib:Download(od, value)
    end
end

--- Gets or sets the mode of an IO.
--- @param io DAQ_IoEnum
--- @param mode? DAQ_IoModeEnum
--- @return DAQ_IoModeEnum?
function DAQ:IoMode(io, mode)
    CheckIo(io)
    if mode == nil then
        return Bitwise.Extract(io, self:IoModes())
    else
        self:IoModes(Bitwise.Inject(io, mode, self.cache.io.mode))
    end
end

--- Gets or sets the state of the IOs.
--- @param value? DAQ_IoValueEnum
--- @return DAQ_IoValueEnum?
function DAQ:IoValues(value)
    local od = self.ib.od["IO"]["Value"]
    if (value == nil) then
        self.cache.io.value = self.ib:Upload(od) --[[@as DAQ_IoValueEnum]]
        return self.cache.io.value
    else
        self.cache.io.value = value
        self.ib:Download(od, value)
    end
end

--- Gets or sets the state of an IO.
--- @param io DAQ_IoEnum
--- @param value? DAQ_IoValueEnum
--- @return DAQ_IoValueEnum?
function DAQ:IoValue(io, value)
    CheckIo(io)
    if value == nil then
        return Bitwise.Extract(io, self:IoValues())
    else
        self:IoValues(Bitwise.Inject(io, value, self.cache.io.value))
    end
end

-- Signaling
--- @enum DAQ_SignalingModeEnum
DAQ.SignalingModeEnum = { off = 0, idle = 1, standby = 2, busy = 3, concern = 4, lockOut = 5, custom = 6 }
--- @enum DAQ_SignalingBuzzerPatternEnum
DAQ.SignalingBuzzerPatternEnum = {
    off = 0,
    on100msPer3s = 1,
    on500msPer1s = 2,
    on900msPer1s = 3,
    on100msPer1s = 4,
    on125msPer250ms = 5
}

--- Gets or set the signaling mode.
--- @param mode? DAQ_SignalingModeEnum
--- @return DAQ_SignalingModeEnum?
function DAQ:SignalingMode(mode)
    local od = self.ib.od["Signaling"]["Mode"]
    if mode == nil then
        return self.ib:Upload(od) --[[@as DAQ_SignalingModeEnum]]
    else
        CheckField(mode, "mode", IsIntegerIn(mode, 0, 6))
        self.ib:Download(od, mode)
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
    elseif pattern ~= nil and duration == nil then
        error("Invalid duration, nil")
    else
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
local function AdcChannelToTestPoint(channel)
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
--- @param shape DAQ_ImpedanceShapeEnum?
--- @param rangeResistor DAQ_ImpedanceRangeResistorEnum?
--- @param frequency integer?
--- @param amplitude number?
--- @param delay integer?
--- @param samplesToTake integer?
--- @param expectedValue number?
--- @param favorSpeed boolean
--- @return DAQ_ImpedanceResults
function DAQ:Impedances(mode, shape, rangeResistor, frequency, amplitude, delay, samplesToTake, expectedValue, favorSpeed)
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

    if shape == nil then shape = DAQ.ImpedanceDefaults.shape end
    if rangeResistor == nil then rangeResistor = DAQ.ImpedanceDefaults.rangeResistor end
    if frequency == nil then frequency = DAQ.ImpedanceDefaults.frequency end
    if amplitude == nil then amplitude = DAQ.ImpedanceDefaults.amplitude end
    if delay == nil then delay = DAQ.ImpedanceDefaults.delay end
    if samplesToTake == nil then samplesToTake = DAQ.ImpedanceDefaults.samplesToTake end
    if expectedValue == nil then expectedValue = DAQ.ImpedanceDefaults.expectedValue end
    if favorSpeed == nil then favorSpeed = DAQ.ImpedanceDefaults.favorSpeed end

    CheckField(shape, "shape", IsIntegerInOd(shape, odShape))
    CheckField(rangeResistor, "range Resistor", IsIntegerInOd(rangeResistor, odRangeResistor))
    CheckField(frequency, "frequency", IsIntegerInOd(frequency, odFrequency))
    CheckField(amplitude, "amplitude", IsFloatInOd(amplitude, odAmplitude))
    CheckField(delay, "delay", IsIntegerInOd(delay, odDelay))
    CheckField(samplesToTake, "samples to take", IsIntegerInOd(samplesToTake, odSamplesToTake))
    CheckField(expectedValue, "expected value", IsIntegerInOd(expectedValue, odExpectedValue))
    CheckField(favorSpeed, "favorSpeed", IsBoolean(favorSpeed))

    if (rangeResistor == 0 or
            (frequency == 0 and shape ~= DAQ.ImpedanceShapeEnum.dc)) and
        expectedValue == 0 then
        error(
            "Expected Value cannot be set to automatic when Range Resistor or Frequency are as well")
    end

    self.ib:Download(odMode, mode)
    self.ib:Download(odShape, shape)
    self.ib:Download(odRangeResistor, rangeResistor)
    self.ib:Download(odFrequency, frequency)
    self.ib:Download(odAmplitude, amplitude)
    self.ib:Download(odDelay, delay)
    self.ib:Download(odSamplesToTake, samplesToTake)
    self.ib:Download(odExpectedValue, expectedValue)
    self.ib:Download(odFavorSpeed, favorSpeed)
    self.ib:Download(odTrigger, true)

    Utils.SleepFor(100)
    local deadline = 5000 - 100
    while (self.ib:Upload(odTrigger)) do
        Utils.SleepFor(10)
        deadline = deadline - 10
        if deadline <= 0 then error("Impedance timeout") end
    end

    if shape == 0 then shape = self.ib:Upload(odShape) --[[@as DAQ_ImpedanceShapeEnum]] end
    if rangeResistor == 0 then rangeResistor = self.ib:Upload(odRangeResistor) --[[@as DAQ_ImpedanceRangeResistorEnum]] end
    if frequency == 0 then frequency = self.ib:Upload(odFrequency) --[[@as integer]] end
    if amplitude == 0 then amplitude = self.ib:Upload(odAmplitude) --[[@as number]] end
    if delay == 0 then delay = self.ib:Upload(odDelay) --[[@as integer]] end
    if samplesToTake == 0 then samplesToTake = self.ib:Upload(odSamplesToTake) --[[@as integer]] end
    if expectedValue == 0 then expectedValue = self.ib:Upload(odExpectedValue) --[[@as integer]] end

    return {
        mode = mode,
        shape = shape --[[@as DAQ_ImpedanceShapeEnum]],
        rangeResistor = rangeResistor --[[@as DAQ_ImpedanceRangeResistorEnum]],
        frequency = frequency --[[@as integer]],
        amplitude = amplitude --[[@as number]],
        delay = delay --[[@as integer]],
        samplesToTake = samplesToTake --[[@as integer]],
        expectedValue = expectedValue --[[@as number]],
        favorSpeed = favorSpeed,
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
    if type(point) == "number" then return { point } end
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
    sampleRate = DAQ.AdcSampleRateEnum.f250Hz --[[@as DAQ_AdcSampleRateEnum]]
}

--- Measures a voltage on one or more points.
--- @param points DAQ_RoutingPointsEnum[]|DAQ_RoutingPointsEnum
--- @param channel DAQ_AdcChannelEnum?
--- @param samplesToTake integer?
--- @param gain DAQ_AdcChannelGainEnum?
--- @param sampleRate DAQ_AdcSampleRateEnum?
--- @return DAQ_AdcChannelResults
function DAQ:MeasureVoltage(points, channel, samplesToTake, gain, sampleRate)
    --- @type DAQ_RoutingPointsEnum[]
    points = PointToPoints(points)
    if IsPointsOk(points) then error("Invalid points") end
    if channel == nil then channel = DAQ.MeasureVoltageDefault.channel end
    CheckField(channel, "Channel", channel == DAQ.AdcChannelEnum.adc2 or channel == DAQ.AdcChannelEnum.adc3)
    if samplesToTake == nil then samplesToTake = DAQ.MeasureVoltageDefault.samplesToTake end
    if gain == nil then gain = DAQ.MeasureVoltageDefault.gain end
    if sampleRate == nil then sampleRate = DAQ.MeasureVoltageDefault.sampleRate end

    local route = self:RequestRouting({ table.unpack(points), AdcChannelToTestPoint(channel) })

    if route == -1 then
        error("Unable to connect points to ADC!")
    else
        Log.D("Using bus " .. route)
    end

    self:AdcChannelGain(channel, gain)
    self:AdcSampleRate(sampleRate)
    self:AdcSamplesToTake(samplesToTake)

    while samplesToTake ~= 0 do
        local previous = samplesToTake
        TimeoutFunction(function()
            local current = self:AdcSamplesToTake() --[[@as integer]]
            local same = current == previous
            previous = current
            return same
        end, 1000)
        samplesToTake = previous
    end

    self:ClearBus(route)

    return self:AdcChannelResults(channel)
end

--- Measures a resistor.
--- @param impP DAQ_RoutingPointsEnum|DAQ_RoutingPointsEnum[]
--- @param impN DAQ_RoutingPointsEnum|DAQ_RoutingPointsEnum[]
--- @param range DAQ_ImpedanceRangeResistorEnum?
--- @param guards DAQ_RoutingPointsEnum[]?
--- @param voltage number?
--- @param delay integer?
--- @param samplesToTake integer?
--- @param favorSpeed boolean
--- @return DAQ_ImpedanceResults
function DAQ:MeasureResistor(impP, impN, range, guards, voltage, delay, samplesToTake, favorSpeed)
    impP = PointToPoints(impP)
    impN = PointToPoints(impN)

    local rimpp = self:RequestRouting({ table.unpack(impP), DAQ.RoutingPointsEnum.IMP_P, DAQ.RoutingPointsEnum.ADC_CH1 })
    local rimpn = self:RequestRouting({ table.unpack(impN), DAQ.RoutingPointsEnum.IMP_N })
    local rguards = nil
    if guards ~= nil then
        guards = PointToPoints(guards)
        rguards = self:RequestRouting({ table.unpack(guards), DAQ.RoutingPointsEnum.GUARD })
    end

    local result = self:Impedances(DAQ.ImpedanceModeEnum.resistor,
        DAQ.ImpedanceShapeEnum.dc, range,
        DAQ.ImpedanceDefaults.frequency, voltage,
        delay // 1, samplesToTake,
        DAQ.ImpedanceDefaults.expectedValue,
        favorSpeed)

    -- cleanup
    self:ClearBus(rimpp)
    self:ClearBus(rimpn)
    if rguards ~= nil then self:ClearBus(rguards) end

    return result
end
