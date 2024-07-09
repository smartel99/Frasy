local Ib = require("lua/core/sdk/environment/ib")
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
local TimeoutFunction = require("lua/core/utils/timeout")
local CheckField = require("lua/core/utils/check_field")

---@class DAQ
DAQ = { ib = nil, cache = { io = { mode = 0, value = 0 } } }
DAQ.__index = DAQ

---Instantiates a DAQ card.
---@param name string|nil Name of the card
---@param nodeId integer|nil ID of the Node
---@return DAQ
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
DAQ.RoutingBusEnum = { all = 0, bus1 = 1, bus2 = 2, bus3 = 3, bus4 = 4 }
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
local function RoutingBusEnumToString(bus)
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

function DAQ:RequestRouting(points)
    if type(points) ~= "table" then error("Invalid points: " .. tostring(points)) end
    if #points < 2 then error("Invalid points count: " .. tostring(#points)) end
    local sPoints = {}
    for k, v in ipairs(points) do
        if not IsInteger(v) then error("Invalid point: [" .. k .. "] " .. tostring(v)) end
        table.insert(sPoints, v)
    end
    table.insert(sPoints, 0)
    self.id:Download(self.id.od["Routing"]["Request Routing"], sPoints)
    -- Might need a delay
    return self.id:Upload(self.id.od["Routing"]["Last Result"])
end

function DAQ:GetBusState(bus)
    if not IsIntegerIn(bus, DAQ.RoutingBusEnum.bus1, DAQ.RoutingBusEnum.bus4) then
        error("Invalid bus: " .. tostring(bus))
    end
    return self.id:Upload(self.id.od["Routing"][RoutingBusEnumToString(bus) .. " State"])
end

function DAQ:ClearBus(bus)
    if not IsIntegerIn(bus, DAQ.RoutingBusEnum.all, DAQ.RoutingBusEnum.bus4) then
        error("Invalid bus: " .. tostring(bus))
    end
    self.id:Download(self.id.od["Routing"]["Clear " .. RoutingBusEnumToString(bus)], true)
end

-- DAC
DAQ.DacShape = { dc = 0, sine = 1, sawtooth = 2, triangle = 3, square = 4, noise = 5 }

function DAQ:DacEnable(state)
    local od = self.id.od["DAC"]["Enable"]
    if state == nil then
        return self.id:Upload(od)
    else
        CheckField(state, "enable", IsBoolean(state))
        self.ib:Download(od, state)
    end
end

---Gets or sets the amplitude of the DAC, in volt.
---@param amplitude number|nil
---@return number
function DAQ:DacAmplitude(amplitude)
    local od = self.id.od["DAC"]["Amplitude"]
    if amplitude == nil then
        return self.id:Upload(od)
    else
        CheckField(amplitude, "amplitude", IsFloatInOd(amplitude, od))
        self.ib:Download(od, amplitude)
    end
end

function DAQ:dacFrequency(frequency)
    local od = self.id.od["DAC"]["Frequency"]
    if frequency == nil then
        return self.id:Upload(od)
    else
        CheckField(frequency, "frequency", IsUnsignedInOd(frequency, od))
        self.ib:Download(od, frequency)
    end
end

function DAQ:DacShape(shape)
    local od = self.id.od["DAC"]["Shape"]
    if shape == nil then
        return self.id:Upload(od)
    else
        CheckField(shape, "shape", IsUnsignedIn(shape, DAQ.DacShape.dc, DAQ.DacShape.noise))
        self.ib:Download(od, shape)
    end
end

-- IO
DAQ.IoEnum = { a = 1, b = 2 }
DAQ.IoModeEnum = { input = 0, output = 1 }
DAQ.IoValueEnum = { low = 0, high = 1 }
local function CheckIo(io) CheckField(io, "io", IsIntegerIn(io, DAQ.IoEnum.a, DAQ.IoEnum.b)) end
local function ExtractIoEntry(io, value)
    if io == DAQ.IoEnum.a then
        return value & 1
    else
        return (value >> 1) & 1
    end
end
local function InjectIoEntry(io, value, cache)
    if io == DAQ.IoEnum.a then
        cache = cache & 0x2
        return cache | (value & 1)
    else
        cache = cache & 0x1
        return cache | ((value << 1) & 1)
    end
end

function DAQ:IoModes(value)
    CheckIo(io)
    local od = self.ib.od["IO"]["Mode"]
    if (value == nil) then
        self.cache.io.mode = self.ib:Upload(od)
        return self.cache.io.mode
    else
        self.cache.io.mode = value
        self.ib:Download(od, value)
    end
end

function DAQ:IoMode(io, mode)
    CheckIo(io)
    if mode == nil then
        return ExtractIoEntry(io, self:IoModes())
    else
        self:IoModes(InjectIoEntry(io, value, self.cache.io.mode))
    end
end

function DAQ:IoValues(value)
    CheckIo(io)
    local od = self.ib.od["IO"]["Value"]
    if (value == nil) then
        self.cache.io.value = self.ib:Upload(od)
        return self.cache.io.value
    else
        self.cache.io.value = value
        self.ib:Download(od, value)
    end
end

function DAQ:IoValue(io, value)
    CheckIo(io)
    if mode == nil then
        return ExtractIoEntry(io, self:IoValues())
    else
        self:IoValues(InjectIoEntry(io, value, self.cache.io.value))
    end
end

-- Signaling
DAQ.SignalingModeEnum = { off = 0, idle = 1, standby = 2, busy = 3, concern = 4, lockOut = 5, custom = 6 }
DAQ.SignalingBuzzerPatternEnum = {
    off = 0,
    on100msPer3s = 1,
    on500msPer1s = 2,
    on900msPer1s = 3,
    on100msPer1s = 4,
    on125msPer250ms = 5
}

function DAQ:SignalingMode(mode)
    local od = self.ib.od["Signaling"]["Mode"]
    if mode == nil then
        return self.ib:Upload(od)
    else
        CheckField(mode, "mode", IsIntegerInOd(mode, od))
        self.ib:Download(od, mode)
    end
end

function DAQ:SignalingIdleTime(minutes)
    local od = self.ib.od["Signaling"]["Idle Time"]
    if minutes == nil then
        return self.ib:Upload(od)
    else
        CheckField(minutes, "idle time", IsIntegerInOd(minutes, od))
        self.id:Download(od, minutes)
    end
end

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
        self.id:Download(od, pattern | (duration << 8))
    end
end

-- UUT Ground
function DAQ:UutGround(state)
    local od = self.ib.od["UUT Ground"]
    if state == nil then
        return self.ib:Upload(od)
    else
        CheckField(state, "state", IsBoolean(state))
        self.ib:Download(od, state)
    end
end

-- Internal Can
function DAQ:InternalCanStandby(state)
    local od = self.ib.od["Internal CAN"]["Standby"]
    if state == nil then
        return self.ib:Upload(od)
    else
        CheckField(state, "state", IsBoolean(state))
        self.ib:Download(od, state)
    end
end

function DAQ:InternalCanTerminationResistor(state)
    local od = self.ib.od["Internal CAN"]["Termination Resistor"]
    if state == nil then
        return self.ib:Upload(od)
    else
        CheckField(state, "state", IsBoolean(state))
        self.ib:Download(od, state)
    end
end

-- I2C
DAQ.I2CGpioEnum = {

}

-- ADC
DAQ.AdcChannelEnum = { adc1, adc2, adc3, adc4 }
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
DAQ.AdcChannelGainEnum = { g1 = 0, g2 = 1, g4 = 2, g8 = 3, g16 = 4 }
local function AdcChannelToInt(channel)
    local indexes = {
        [DAQ.AdcChannelEnum.adc1] = 1,
        [DAQ.AdcChannelEnum.adc2] = 2,
        [DAQ.AdcChannelEnum.adc3] = 3,
        [DAQ.AdcChannelEnum.adc4] = 4
    }
    return indexes[channel]
end
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
local function GetAdcChannelOb(channel, affix)
    return self.ib.od["ADC"]["Channel " .. AdcChannelToInt(channel) .. " " .. affix]
end

function DAQ:AdcIdCheck() return self.ib:Upload(self.ib.od["ADC"]["ID Check"]) end

function DAQ:AdcSamplesToTake(count)
    local ob = self.ib.od["ADC"]["Samples to Take"]
    if count == nil then
        return self.ib:Upload(ob)
    else
        CheckField(count, "count", IsIntegerInOd(count, ob))
        self.ib:Download(ob, count)
    end
end

function DAQ:AdcSampleRate(sampleRate)
    local ob = self.ib.od["ADC"]["Sample Rate"]
    if sampleRate == nil then
        return self.ib:Upload(od)
    else
        CheckField(sampleRate, "sample rate", IsIntegerInOd(sampleRate, od))
        self.ib:Download(ob, sampleRate)
    end
end

function DAQ:AdcChannelGain(channel, gain)
    CheckAdcChannel(channel)
    local ob = GetAdcChannelOb(channel, "Gain")
    if gain == nil then
        return self.ib:Upload(ob)
    else
        CheckField(gain, "gain", IsIntegerInOd(gain, od))
        self.ib:Download(ob, gain)
    end
end

function DAQ:AdcChannelResults(channel)
    CheckAdcChannel(channel)
    local obMin = GetAdcChannelOb(channel, "Min")
    local obMax = GetAdcChannelOb(channel, "Max")
    local obAvg = GetAdcChannelOb(channel, "Average")

    return { min = self.ib:Upload(odMin), max = self.ib:Upload(odMax), average = self.ib:Upload(odAvg) }
end

function DAQ:AdcChannelAverage(channel)
    CheckAdcChannel(channel)
    local ob = GetAdcChannelOb(channel, "Average")
    return self.ib:Upload(odAvg)
end

-- ADC Calibration
function DAQ:AdcCalibration()
    local ob = self.ib.od["ADC Calibration"]
    local function f(od) return self.ib:Upload(od) end
    return {
        [1] = {
            gain = {
                r100 = f(ob["Channel 1 Gain 100R"]),
                r4k99 = f(ob["Channel 1 Gain 4k99"]),
                r100k = f(ob["Channel 1 Gain 100k"]),
                r1M = f(ob["Channel 1 Gain 1M"])
            },
            offset = {
                r100 = f(ob["Channel 1 Offset 100R"]),
                r4k99 = f(ob["Channel 1 Offset 4k99"]),
                r100k = f(ob["Channel 1 Offset 100k"]),
                r1M = f(ob["Channel 1 Offset 1M"])
            }
        },
        [2] = {
            gain = {
                base = f(ob["Channel 2 Gain"]),
                r100 = f(ob["Channel 2 Gain 100R"]),
                r4k99 = f(ob["Channel 2 Gain 4k99"]),
                r100k = f(ob["Channel 2 Gain 100k"]),
                r1M = f(ob["Channel 2 Gain 1M"])
            },
            offset = {
                base = f(ob["Channel 2 Offset"]),
                r100 = f(ob["Channel 2 Offset 100R"]),
                r4k99 = f(ob["Channel 2 Offset 4k99"]),
                r100k = f(ob["Channel 2 Offset 100k"]),
                r1M = f(ob["Channel 2 Offset 1M"])
            }
        },
        [3] = { gain = f(ob["Channel 3 Gain"]), offset = f(ob["Channel 3 Offset"]) }
    }
end

-- Impedances
DAQ.ImpedanceModeEnum = { resistor = 0, capacitor = 1, inductor = 2 }
DAQ.ImpedanceShapeEnum = {
    automatic = 0,
    dc = 1,
    sine = 2,
    triangle = 3,
    ramp = 4,
    noise = 5
}
DAQ.ImpedanceRangeResistorEnum = {
    automatic = 0,
    r100 = 1,
    r4k99 = 2,
    r100k = 3,
    r1M = 4
}
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

function DAQ:Impedances(mode, shape, rangeResistor, frequency, amplitude, delay, samplesToTake, expectedValue,
                        favorSpeed)
    CheckField(mode, "mode", IsIntegerInOd(mode, odMode))

    local od = self.ib.od["Impedances"]
    local odTrigger = od["Trigger"]

    if self.ib:Upload(odTrigger) then error("Impedance measure already running") end

    local odMode = od["Mode"]
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

    if shape == 0 then shape = self.ib:Upload(odShape) end
    if rangeResistor == 0 then rangeResistor = self.ib:Upload(odRangeResistor) end
    if frequency == 0 then frequency = self.ib:Upload(odFrequency) end
    if amplitude == 0 then amplitude = self.ib:Upload(odAmplitude) end
    if delay == 0 then delay = self.ib:Upload(odDelay) end
    if samplesToTake == 0 then samplesToTake = self.ib:Upload(odSamplesToTake) end
    if expectedValue == 0 then expectedValue = self.ib:Upload(odExpectedValue) end

    return {
        mode = mode,
        shape = shape,
        rangeResistor = rangeResistor,
        frequency = frequency,
        amplitude = amplitude,
        delay = delay,
        samplesToTake = samplesToTake,
        expectedValue = expectedValue,
        favorSpeed = favorSpeed,
        confidence = self.ib:Upload(odConfidence),
        value = self.ib:Upload(odValue),
        vin = self.ib:Upload(odVin),
        vout = self.ib:Upload(odVout)
    }
end

-- High level functions
local function PointToPoints(point)
    if type(point) == "number" then return { point } end
    return point
end

local function IsPointsOk(points)
    if type(points) ~= table and #points == 0 then return false end
    for _, v in ipairs(points) do if not IsIntegerIn(points, 0, 75) then return false end end
    return true
end

DAQ.MeasureVoltageDefault = {
    channel = DAQ.AdcChannelEnum.adc2,
    samplesToTake = 10,
    gain = DAQ.AdcChannelGainEnum.g1,
    sampleRate = DAQ.AdcSampleRateEnum.f250Hz
}

---Measures a voltage on one or more points.
---@param points table|DAQ_TestPoints
---@param channel DAQ_AdcChannelEnum|nil
---@param samplesToTake integer|nil
---@param gain DAQ_AdcChannelGainEnum|nil
---@param sampleRate DAQ_AdcSampleRateEnum|nil
---@return DAQ_AdcChannelResults
function DAQ:MeasureVoltage(points, channel, samplesToTake, gain, sampleRate)
    points = PointToPoints(points)
    if IsPointsOk(points) then error("Invalid points") end
    if channel == nil then channel = DAQ.MeasureVoltageDefault.channel end
    CheckField(channel, "Channel", function(ch) return ch == DAQ.AdcChannelEnum.adc2 or ch == DAQ.AdcChannelEnum.adc3 end)
    if samplesToTake == nil then samplesToTake = DAQ.MeasureVoltageDefault.samplesToTake end
    if gain == nil then gain = DAQ.MeasureVoltageDefault.gain end
    if sampleRate == nil then sampleRate = DAQ.MeasureVoltageDefault.sampleRate end

    table.insert(points, channel)
    local route = self:RequestRouting(points)

    self:AdcChannelGain(channel, gain)
    self:AdcChannelSampleRate(channel, sampleRate)
    self:AdcSamplesToTake(samplesToTake)

    while samplesToTake ~= 0 do
        local previous = samplesToTake
        TimeoutFunction(function()
            local current = self:AdcSamplesToTake()
            local same = current == previous
            previous = current
            return same
        end, 1000)
        samplesToTake = previous
    end

    self:ClearBus(route)

    return self:AdcChannelResults(channel)
end

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
