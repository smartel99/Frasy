local Ib = require("lua/core/sdk/environment/ib")
local isBoolean = require("lua.core.utils.is_boolean.is_boolean")
local isInteger = require("lua.core.utils.is_integer.is_integer")
local isIntegerIn = require("lua.core.utils.is_integer.is_integer_in")
local isIntegerInOd = function(value, od)
    return isIntegerInOd(value, od.LowLimit, od.HighLimit)
end
local isUnsigned = require("lua.core.utils.is_unsigned.is_unsigned")
local isUnsignedIn = require("lua.core.utils.is_unsigned.is_unsigned_in")
local isUnsignedInOd = function(value, od)
    return isUnsignedIn(value, od.LowLimit, od.HighLimit)
end
local isFloat = require("lua.core.utils.is_float.is_float")
local isFloatIn = require("lua.core.utils.is_float.is_floatIn")
local isFloatInOd = function(value, od)
    return isFloatIn(value, od.LowLimit, od.HighLimit)
end
DAQ = {ib = nil, cache = {io = {mode = 0, value = 0}}}
DAQ.__index = DAQ

function DAQ:new(name, nodeId)
    local ib = Ib:new()
    ib.kind = 02;
    if name == nil then name = "daq" end
    ib.name = name
    if nodeId == nil then nodeId = ib.kind end
    ib.nodeId = nodeId
    ib.eds = "lua/core/cep/eds/daq_1.0.0.eds"
    return setmetatable({ib = ib, cache = {io = {mode = 0, value = 0}}}, DAQ)
end

local function checkField(name, value, predicate)
    assert(predicate, "Invalid " .. name .. ": " .. tostring(value))
end

-- Routing
DAQ.RoutingBusEnum = {all = 0, bus1 = 1, bus2 = 2, bus3 = 3, bus4 = 4}
local function routingBusEnumToString(bus)
    if not isIntegerIn(bus, DAQ.RoutingBusEnum.all, DAQ.RoutingBusEnum.bus4) then
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

function DAQ:requestRouting(points)
    if type(points) ~= "table" then
        error("Invalid points: " .. tostring(points))
    end
    if #points < 2 then error("Invalid points count: " .. tostring(#points)) end
    local sPoints = {}
    for k, v in ipairs(points) do
        if not isInteger(v) then
            error("Invalid point: [" .. k .. "] " .. tostring(v))
        end
        table.insert(sPoints, v)
    end
    table.insert(sPoints, 0)
    self.id:Download(self.id.od["Routing"]["Request Routing"], sPoints)
    -- Might need a delay
    return self.id:Upload(self.id.od["Routing"]["Last Result"])
end

function DAQ:getBusState(bus)
    if not isIntegerIn(bus, DAQ.RoutingBusEnum.bus1, DAQ.RoutingBusEnum.bus4) then
        error("Invalid bus: " .. tostring(bus))
    end
    return self.id:Upload(self.id.od["Routing"][routingBusEnumToString(bus) ..
                              " State"])
end

function DAQ:clearBus(bus)
    if not isIntegerIn(bus, DAQ.RoutingBusEnum.all, DAQ.RoutingBusEnum.bus4) then
        error("Invalid bus: " .. tostring(bus))
    end
    self.id:Download(self.id.od["Routing"]["Clear " ..
                         routingBusEnumToString(bus)], true)
end

-- DAC
DAQ.DacShape = {
    dc = 0,
    sine = 1,
    sawtooth = 2,
    triangle = 3,
    square = 4,
    noise = 5
}

function DAQ:dacEnable(state)
    local od = self.id.od["DAC"]["Enable"]
    if state == nil then
        return self.id:Upload(od)
    else
        checkField("enable", state, isBoolean(state))
        self.ib:Download(od, state)
    end
end

function DAQ:dacAmplitude(amplitude)
    local od = self.id.od["DAC"]["Amplitude"]
    if amplitude == nil then
        return self.id:Upload(od)
    else
        checkField("amplitude", amplitude, isFloatInOd(amplitude, od))
        self.ib:Download(od, amplitude)
    end
end

function DAQ:dacFrequency(frequency)
    local od = self.id.od["DAC"]["Frequency"]
    if frequency == nil then
        return self.id:Upload(od)
    else
        checkField("frequency", frequency, isUnsignedInOd(frequency, od))
        self.ib:Download(od, frequency)
    end
end

function DAQ:dacShape(shape)
    local od = self.id.od["DAC"]["Shape"]
    if shape == nil then
        return self.id:Upload(od)
    else
        checkField("shape", shape,
                   isUnsignedIn(shape, DAQ.DacShape.dc, DAQ.DacShape.noise))
        self.ib:Download(od, shape)
    end
end

-- IO
DAQ.IoEnum = {a = 1, b = 2}
DAQ.IoModeEnum = {input = 0, output = 1}
DAQ.IoValueEnum = {low = 0, high = 1}
local function checkIo(io)
    checkField("io", io, isIntegerIn(io, DAQ.IoEnum.a, DAQ.IoEnum.b))
end
local function extractIoEntry(io, value)
    if io == DAQ.IoEnum.a then
        return value & 1
    else
        return (value >> 1) & 1
    end

end
local function injectIoEntry(io, value, cache)
    if io == DAQ.IoEnum.a then
        cache = cache & 0x2
        return cache | (value & 1)
    else
        cache = cache & 0x1
        return cache | ((value << 1) & 1)
    end
end

function DAQ:ioModes(value)
    checkIo(io)
    local od = self.ib.od["IO"]["Mode"]
    if (value == nil) then
        self.cache.io.mode = self.ib:Upload(od)
        return self.cache.io.mode
    else
        self.cache.io.mode = value
        self.ib:Download(od, value)
    end
end
function DAQ:ioMode(io, mode)
    checkIo(io)
    if mode == nil then
        return extractIoEntry(io, self:ioModes())
    else
        self:ioModes(injectIoEntry(io, value, self.cache.io.mode))
    end
end

function DAQ:ioValues(value)
    checkIo(io)
    local od = self.ib.od["IO"]["Value"]
    if (value == nil) then
        self.cache.io.value = self.ib:Upload(od)
        return self.cache.io.value
    else
        self.cache.io.value = value
        self.ib:Download(od, value)
    end
end
function DAQ:ioValue(io, value)
    checkIo(io)
    if mode == nil then
        return extractIoEntry(io, self:ioValues())
    else
        self:ioValues(injectIoEntry(io, value, self.cache.io.value))
    end
end

-- Signaling
DAQ.SignalingModeEnum = {
    off = 0,
    idle = 1,
    standby = 2,
    busy = 3,
    concern = 4,
    lockOut = 5,
    custom = 6
}
DAQ.SignalingBuzzerPatternEnum = {
    off = 0,
    on100msPer3s = 1,
    on500msPer1s = 2,
    on900msPer1s = 3,
    on100msPer1s = 4,
    on125msPer250ms = 5
}

function DAQ:signalingMode(mode)
    local od = self.ib.od["Signaling"]["Mode"]
    if mode == nil then
        return self.ib:Upload(od)
    else
        checkField("mode", mode, isIntegerInOd(mode, od))
        self.ib:Download(od, mode)
    end
end

function DAQ:signalingIdleTime(minutes)
    local od = self.ib.od["Signaling"]["Idle Time"]
    if minutes == nil then
        return self.ib:Upload(od)
    else
        checkField("idle time", minutes, isIntegerInOd(minutes, od))
        self.id:Download(od, minutes)
    end
end

function DAQ:signalingBuzzerMode(pattern, duration)
    local od = self.ib.od["Signaling"]["Buzzer Mode"]
    if pattern == nil and duration == nil then
        local result = self.ib:Upload(od)
        return {pattern = result & 0xFF, duration = (result >> 8) & 0xFF}
    elseif pattern == nil and duration ~= nil then
        error("Invalid pattern, nil")
    elseif pattern ~= nil and duration == nil then
        error("Invalid duration, nil")
    else
        checkField("pattern", pattern,
                   isIntegerIn(pattern, DAQ.SignalingBuzzerPatternEnum.off,
                               DAQ.SignalingBuzzerPatternEnum.on125msPer250ms))
        checkField("duration", duration, isIntegerIn(duration, 0, 255))
        self.id:Download(od, pattern | (duration << 8))
    end
end

-- UUT Ground
function DAQ:uutGround(state)
    local od = self.ib.od["UUT Ground"]
    if state == nil then
        return self.ib:Upload(od)
    else
        checkField("state", state, isBoolean(state))
        self.ib:Download(od, state)
    end
end

-- Internal Can
function DAQ:internalCanStandby(state)
    local od = self.ib.od["Internal CAN"]["Standby"]
    if state == nil then
        return self.ib:Upload(od)
    else
        checkField("state", state, isBoolean(state))
        self.ib:Download(od, state)
    end
end
function DAQ:internalCanTerminationResistor(state)
    local od = self.ib.od["Internal CAN"]["Termination Resistor"]
    if state == nil then
        return self.ib:Upload(od)
    else
        checkField("state", state, isBoolean(state))
        self.ib:Download(od, state)
    end
end

-- ADC
DAQ.AdcChannelEnum = {adc1, adc2, adc3, adc4}
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
DAQ.AdcChannelGainEnum = {g1 = 0, g2 = 1, g4 = 2, g8 = 3, g16 = 4}
local function adcChannelToInt(channel)
    local indexes = {
        [DAQ.AdcChannelEnum.adc1] = 1,
        [DAQ.AdcChannelEnum.adc2] = 2,
        [DAQ.AdcChannelEnum.adc3] = 3,
        [DAQ.AdcChannelEnum.adc4] = 4
    }
    return indexes[channel]
end
local function checkAdcChannel(channel)
    checkField("channel", channel, isIntegerIn(channel, DAQ.AdcChannelEnum.adc1,
                                               DAQ.AdcChannelEnum.adc4))
end
local function getAdcChannelOb(channel, affix)
    return self.ib.od["ADC"]["Channel " .. adcChannelToInt(channel) .. " " ..
               affix]
end

function DAQ:adcIdCheck() return self.ib:Upload(self.ib.od["ADC"]["ID Check"]) end

function DAQ:adcSamplesToTake(count)
    local ob = self.ib.od["ADC"]["Samples to Take"]
    if count == nil then
        return self.ib:Upload(ob)
    else
        checkField("count", count, isIntegerInOd(count, ob))
        self.ib:Download(ob, count)
    end
end

function DAQ:adcSampleRate(sampleRate)
    local ob = self.ib.od["ADC"]["Sample Rate"]
    if sampleRate == nil then
        return self.ib:Upload(od)
    else
        checkField("sample rate", sampleRate, isIntegerInOd(sampleRate, od))
        self.ib:Download(ob, sampleRate)
    end
end

function DAQ:adcChannelGain(channel, gain)
    checkAdcChannel(channel)
    local ob = getAdcChannelOb(channel, "Gain")
    if gain == nil then
        return self.ib:Upload(ob)
    else
        checkField("gain", gain, isIntegerInOd(gain, od))
        self.ib:Download(ob, gain)
    end
end

function DAQ:adcChannelResults(channel)
    checkAdcChannel(channel)
    local obMin = getAdcChannelOb(channel, "Min")
    local obMax = getAdcChannelOb(channel, "Max")
    local obAvg = getAdcChannelOb(channel, "Average")

    return {
        min = self.ib:Upload(odMin),
        max = self.ib:Upload(odMax),
        average = self.ib:Upload(odAvg)
    }
end

function DAQ:adcChannelAverage(channel)
    checkAdcChannel(channel)
    local ob = getAdcChannelOb(channel, "Average")
    return self.ib:Upload(odAvg)
end

-- ADC Calibration
function DAQ:adcCalibration()
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
        [3] = {
            gain = f(ob["Channel 3 Gain"]),
            offset = f(ob["Channel 3 Offset"])
        }
    }
end

-- Impedances
DAQ.ImpedanceModeEnum = {resistor = 0, capacitor = 1, inductor = 2}
DAQ.ImpendanceShapeEnum = {
    automatic = 0,
    dc = 1,
    sine = 2,
    triangle = 3,
    ramp = 4,
    noise = 5
}
DAQ.ImpedanceRangeRegistorEnum = {
    automatic = 0,
    r100 = 1,
    r4k99 = 2,
    r100k = 3,
    r1M = 4
}
function DAQ:Impedances(mode, shape, rangeRegistor, frequency, amplitude, delay,
                        samplesToTake, expectedValue, favorSpeed)
    checkField("mode", mode, isIntegerInOd(mode, odMode))

    local od = self.ib.od["Impedances"]
    local odTrigger = od["Trigger"]

    if self.ib:Upload(odTrigger) then
        error("Impedance measure already running")
    end

    local odMode = od["Mode"]
    local odShape = od["Shape"]
    local odRangeRegistor = od["Range Registor"]
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

    if shape == nil then shape = DAQ.ImpendanceShapeEnum.automatic end
    if rangeRegistor == nil then
        rangeRegistor = DAQ.ImpedanceRangeRegistorEnum.automatic
    end
    if frequency == nil then frequency = 0 end
    if amplitude == nil then amplitude = 0 end
    if delay == nil then delay = 0 end
    if samplesToTake == nil then samplesToTake = 0 end
    if expectedValue == nil then expectedValue = 0 end
    if favorSpeed == nil then favorSpeed = false end

    checkField("shape", shape, isIntegerInOd(shape, odShape))
    checkField("range registor", rangeRegistor,
               isIntegerInOd(rangeRegistor, odRangeRegistor))
    checkField("frequency", frequency, isIntegerInOd(frequency, odFrequency))
    checkField("amplitude", amplitude, isFloatInOd(amplitude, odAmplitude))
    checkField("delay", delay, isIntegerInOd(delay, odDelay))
    checkField("samples to take", samplesToTake,
               isIntegerInod(samplesToTake, odSamplesToTake))
    checkField("expected value", expectedValue,
               isIntegerInOd(expectedValue, odExpectedValue))
    checkField("favorSpeed", favorSpeed, isBoolean(favorSpeed))

    if (rangeRegistor == 0 or frequency == 0) and expectedValue == 0 then
        error(
            "Expected Value cannot be set to automatic when Range registor or Frequency are as well")
    end

    self.ib:Download(odMode, mode)
    self.ib:Download(odShape, shape)
    self.ib:Download(odRangeRegistor, rangeRegistor)
    self.ib:Download(odFrequency, frequency)
    self.ib:Download(odAmplitude, amplitude)
    self.ib:Download(odDelay, delay)
    self.ib:Download(odSamplesToTake, samplesToTake)
    self.ib:Download(odExpectedValue, expectedValue)
    self.ib:Download(odFavorSpeed, favorSpeed)
    self.ib:Download(odTrigger, true)

    Utils.sleep_for(100)
    local deadline = 5000 - 100
    while (self.ib:Upload(odTrigger)) do
        Utils.sleep_for(10)
        deadline = deadline - 10
        if deadline <= 0 then error("Impedance timeout") end
    end

    if shape == 0 then shape = self.ib:Upload(odShape) end
    if rangeRegistor == 0 then
        rangeRegistor = self.ib:Upload(odRangeRegistor)
    end
    if frequency == 0 then frequency = self.ib:Upload(odFrequency) end
    if amplitude == 0 then amplitude = self.ib:Upload(odAmplitude) end
    if delay == 0 then delay = self.ib:Upload(odDelay) end
    if samplesToTake == 0 then
        samplesToTake = self.ib:Upload(odSamplesToTake)
    end
    if expectedValue == 0 then
        expectedValue = self.ib:Upload(odExpectedValue)
    end

    return {
        mode = mode,
        shape = shape,
        rangeRegistor = rangeRegistor,
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
