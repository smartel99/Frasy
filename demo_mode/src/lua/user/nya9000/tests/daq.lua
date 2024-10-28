local CheckField = require("lua/core/utils/check_field")
local IsIntegerIn = require("lua/core/utils/is_integer/is_integer_in")

Sequence("DAQ", function()
    ---@type DAQ
    local daq = Context.map.ibs.daq
    
    -- Test("Routing", function()
    --     ---@type DAQ
    --     local daq = Context.map.ibs.daq

    --     local route = daq:RequestRouting({ DAQ.RoutingPointsEnum.P5V,
    --         DAQ.RoutingPointsEnum.ADC_CH1 })
    --     if route == -1 then
    --         error("Unable to connect route")
    --     else
    --         Log.D("Using bus " .. route)
    --     end

    --     local popup = Popup("Routing")
    --     popup:Show()

    --     daq:ClearBus(route)
    -- end)

    -- Test("ADC", function()
    --     ---@type DAQ
    --     local ib = Context.map.ibs.daq

    --     local res = ib:MeasureVoltage({DAQ.RoutingPointsEnum.P2V048}, DAQ.AdcChannelEnum.adc2)
    --     Log.I("Min: "..res.min)
    --     Log.I("Avg: "..res.average)
    --     Log.I("Max: "..res.max)
    -- end)

    local ios = { "a", "b", "spiUutSck", "spiUutMosi", "spiUutMiso", "i2cUutScl", "i2cUutSda", "uartUutTx", "uartUutRx" }

    -- Test("IO Inputs", function()
    --     ---@type DAQ
    --     local daq = Context.map.ibs.daq

    --     daq:IoModes(0)

    --     for _, io in ipairs(ios) do
    --         local popup = Popup(io):Text("Set IO to high")
    --         popup:Routine(function()
    --             Log.D("asdf")
    --             if daq:IoValue(DAQ.IoEnum[io]) == DAQ.IoValueEnum.high then
    --                 popup:Consume()
    --             end
    --         end)
    --         popup:Show()
    --     end
    -- end)

    -- Test("IO Outputs", function()
    --     --- @type DAQ
    --     local daq = Context.map.ibs.daq

    --     daq:IoModes(0x1FF)

    --     for _, io in ipairs(ios) do
    --         daq:IoValue(DAQ.IoEnum[io], DAQ.IoValueEnum.high)
    --         local popupHigh = Popup(io .. " High"):Text("Check that IO is high")
    --         popupHigh:Show()

    --         daq:IoValue(DAQ.IoEnum[io], DAQ.IoValueEnum.low)
    --         local popupLow = Popup(io .. " Low"):Text("Check that IO is Low")
    --         popupLow:Show()
    --     end
    -- end)

    -- Test("Routing", function()
    --     if Context.info.stage ~= Stage.execution then return end
    --     --- @type DAQ
    --     local daq = Context.map.ibs.daq
    --     for run = 0, 100 do
    --         for i = DAQ.RoutingPointsEnum.MUX1_A0, DAQ.RoutingPointsEnum.MUX6_B3 do
    --             if not (i == DAQ.RoutingPointsEnum.MUX1_OUT or
    --                     i == DAQ.RoutingPointsEnum.MUX2_OUT or
    --                     i == DAQ.RoutingPointsEnum.MUX3_OUT or
    --                     i == DAQ.RoutingPointsEnum.MUX4_OUT or
    --                     i == DAQ.RoutingPointsEnum.MUX5_OUT or
    --                     i == DAQ.RoutingPointsEnum.MUX6_OUT) then
    --                 local route = daq:RequestRouting({ DAQ.RoutingPointsEnum.P3V3, i })
    --                 daq:ClearBus(route)
    --             end
    --         end
    --     end
    -- end)

    Test("Stress test comm", function()
        local repeatCount = 1
        local epoch = 0
        if Context.info.stage == Stage.execution then repeatCount = 1000 end
        for i = 1, repeatCount do
            local start = os.clock()
            -- Measure AC_MEAN with 10x divider. (0VAC @ 10%)
            local result = daq:MeasureVoltage(DAQ.RoutingPointsEnum.P5V, nil, 533, nil, DAQ.AdcSampleRateEnum.f16000Hz)
            local delta =os.clock() - start
            Log.D("Epoch " .. tostring(i) .. " in " .. delta .. "s")
            Expect(delta, "Time"):ToBeGreater(0.0)

        end
    end)
    -- Test("Stress test voltmeter", function()
    --     ---@type DAQ
    --     local daq = Context.map.ibs.daq

    --     local sampleRates = {
    --         DAQ.AdcSampleRateEnum.f250Hz,
    --         DAQ.AdcSampleRateEnum.f500Hz,
    --         DAQ.AdcSampleRateEnum.f1000Hz,
    --         DAQ.AdcSampleRateEnum.f2000Hz,
    --         DAQ.AdcSampleRateEnum.f2560Hz,
    --         DAQ.AdcSampleRateEnum.f2667Hz,
    --         DAQ.AdcSampleRateEnum.f4000Hz,
    --         DAQ.AdcSampleRateEnum.f5120Hz,
    --         DAQ.AdcSampleRateEnum.f5333Hz,
    --         DAQ.AdcSampleRateEnum.f8000Hz,
    --         DAQ.AdcSampleRateEnum.f10240Hz,
    --         DAQ.AdcSampleRateEnum.f10667Hz,
    --         DAQ.AdcSampleRateEnum.f16000Hz,
    --         DAQ.AdcSampleRateEnum.f20480Hz,
    --         DAQ.AdcSampleRateEnum.f21333Hz,
    --         DAQ.AdcSampleRateEnum.f32000Hz,
    --     }
    --     local sampleRatesStr = {
    --         [0] = "0.25ksps",
    --         [1] = "0.5ksps",
    --         [2] = "1ksps",
    --         [3] = "2ksps",
    --         [4] = "2.560ksps",
    --         [5] = "2.667ksps",
    --         [6] = "4ksps",
    --         [7] = "5.120ksps",
    --         [8] = "5.333ksps",
    --         [9] = "8ksps",
    --         [10] = "10.24ksps",
    --         [11] = "10.667ksps",
    --         [12] = "16ksps",
    --         [13] = "20.48ksps",
    --         [14] = "21.333ksps",
    --         [15] = "32ksps"
    --     }

    --     local pass = true
    --     local loops = 0
    --     while pass do
    --         loops = loops + 1
    --         local samples = 10 --math.random(3, 500)
    --         local sampleRate = sampleRates[math.random(1, 16)]
    --         local res = daq:MeasureVoltage({ DAQ.RoutingPointsEnum.P2V048 }, DAQ.AdcChannelEnum.adc2, samples, nil,
    --             sampleRate)
    --         Log.I(string.format("Run %d: Measured %0.6fV (min: %0.6fV, max %0.6fV), %d samples, sample rate: %s", loops,
    --             res.average,
    --             res.min,
    --             res.max, samples, sampleRatesStr[sampleRate]))
    --         Expect(res.average, "Volt"):ToBeInRange(2.00, 2.08)
    --         if res.average < 2.00 or res.average > 2.08 then
    --             Log.E("Out of spec!")
    --             pass = false
    --         end
    --         if loops >= 1000 then pass = false end
    --     end
    -- end)

    -- Test("Stress test resistance", function()
    --     ---@type DAQ
    --     local daq = Context.map.ibs.daq

    --     local pass = true
    --     local loops = 0
    --     while pass do
    --         loops = loops + 1
    --         local imp = daq:MeasureResistor(DAQ.RoutingPointsEnum.MUX1_A0, DAQ.RoutingPointsEnum.MUX2_A0,
    --             1000)
    --         Log.I(string.format("Run %d: Measured %d ohms (vin: %0.6fV, Vout %0.6fV)", loops, imp.value,
    --             imp.vin,
    --             imp.vout))
    --         Expect(imp.value, "Impedance"):ToBeInRange(900, 1100)
    --         if imp.value < 900 or imp.value > 1100 then
    --             Log.E("Out of spec!")
    --             pass = false
    --         end
    --         if loops >= 200 then pass = false end
    --     end
    -- end)

    -- Test("Resistance Measurements", function()
    --     ---@type DAQ
    --     local daq = Context.map.ibs.daq

    --     local results = {
    --         [DAQ.ImpedanceRangeResistorEnum.r100] = {},
    --         [DAQ.ImpedanceRangeResistorEnum.r4k99] = {},
    --         [DAQ.ImpedanceRangeResistorEnum.r100k] = {},
    --         [DAQ.ImpedanceRangeResistorEnum.r1M] = {}
    --     }
    --     local expecteds = {
    --         1, 8, 47, 100, 497, 1003, 4991, 9980,
    --         49900, 100000, 248800, 498700, 748000, 998000, 1998000,
    --         4998000, 10030000
    --     }
    --     local ranges = {
    --         DAQ.ImpedanceRangeResistorEnum.r100,
    --         DAQ.ImpedanceRangeResistorEnum.r4k99,
    --         DAQ.ImpedanceRangeResistorEnum.r100k,
    --         DAQ.ImpedanceRangeResistorEnum.r1M
    --     }

    --     -- For each expected:
    --     --  popup, request res change
    --     --  do 25 times:
    --     --    For each range:
    --     --      Measure impedance, save results.
    --     -- Print results.

    --     for i, expected in ipairs(expecteds) do
    --         results[DAQ.ImpedanceRangeResistorEnum.r100][expected] = {}
    --         results[DAQ.ImpedanceRangeResistorEnum.r4k99][expected] = {}
    --         results[DAQ.ImpedanceRangeResistorEnum.r100k][expected] = {}
    --         results[DAQ.ImpedanceRangeResistorEnum.r1M][expected] = {}
    --         local popup = Popup("Res"):Text("Put the " .. expected .. "ohms resistor")
    --         popup:Show()
    --         for j = 1, 25 do
    --             for k, range in ipairs(ranges) do
    --                 local imp = daq:MeasureResistor(DAQ.RoutingPointsEnum.MUX1_A0, DAQ.RoutingPointsEnum.MUX2_A0,
    --                     expected, range)
    --                 table.insert(results[range][expected], { vin = imp.vin, vout = imp.vout, value = imp.value })
    --             end
    --         end
    --     end

    --     for i, range in ipairs(ranges) do
    --         print("Range " .. range)
    --         for j, expected in ipairs(expecteds) do
    --             print("  " .. expected)
    --             for k, v in ipairs(results[range][expected]) do
    --                 print("    " .. v.value .. "," .. v.vin .. "," .. v.vout)
    --             end
    --         end
    --     end
    -- end)

    -- Test("Upload", function()
    --     local daq = Context.map.ibs.daq
    --     for i = 0, 250 do
    --         daq.ib:Upload(daq.ib.od["Routing"]["Last Result"])
    --     end
    -- end)

    -- Test("Download", function()
    --     ---@type DAQ
    --     local daq = Context.map.ibs.daq
    --     for i = 0, 250 do
    --         daq.ib:Download(daq.ib.od["IO"]["Mode"], 0)
    --     end
    -- end)

    -- Test("SDO stress test", function()
    --     ---@type DAQ
    --     local daq = Context.map.ibs.daq
    --     for i=0,250 do
    --         daq.ib:Download(daq.ib.od["IO"]["Mode"], 0)
    --         daq.ib:Upload(daq.ib.od["Routing"]["Last Result"])
    --     end
    -- end)

    -- Test("Signaling", function()
    --     ---@type DAQ
    --     local daq = Context.map.ibs.daq

    --     local modes = {
    --         DAQ.SignalingModeEnum.idle,
    --         DAQ.SignalingModeEnum.standby,
    --         DAQ.SignalingModeEnum.busy,
    --         DAQ.SignalingModeEnum.concern,
    --         DAQ.SignalingModeEnum.lockOut,
    --         DAQ.SignalingModeEnum.danger,
    --         DAQ.SignalingModeEnum.mutedDanger,
    --         DAQ.SignalingModeEnum.boot,
    --         DAQ.SignalingModeEnum.off,
    --     }
    --     for i, mode in ipairs(modes) do
    --         daq:SignalingMode(mode)
    --         local popup = Popup("Signaling"):Text("Admire the pretty colors~")
    --         popup:Show()
    --     end
    --     daq:SignalingMode(DAQ.SignalingModeEnum.standby)
    -- end)

    -- Test("Signal Modifiers", function()
    --     ---@type DAQ
    --     local daq = Context.map.ibs.daq

    --     local modes = {
    --         DAQ.SignalingModeModifierEnum.on100msPer3s,
    --         DAQ.SignalingModeModifierEnum.on100msPer1s,
    --         DAQ.SignalingModeModifierEnum.on250msPer1s,
    --         DAQ.SignalingModeModifierEnum.on500msPer1s,
    --         DAQ.SignalingModeModifierEnum.on125msPer250ms,
    --         DAQ.SignalingModeModifierEnum.strobe
    --     }

    --     for i, mode in ipairs(modes) do
    --         daq:SignalingModeModifier(mode)
    --         local popup = Popup("Signaling"):Text("Admire the pretty colors~")
    --         popup:Show()
    --     end
    --     daq:SignalingModeModifier(DAQ.SignalingModeModifierEnum.off)
    -- end)

    -- Test("Custom Signaling", function()
    --     ---@type DAQ
    --     local daq = Context.map.ibs.daq
    --     daq:SignalingMode(DAQ.SignalingModeEnum.custom)

    --     local leds = {
    --         DAQ.SignalingLed.led1, DAQ.SignalingLed.led2, DAQ.SignalingLed.led3, DAQ.SignalingLed.led4,
    --         DAQ.SignalingLed.led5, DAQ.SignalingLed.led6, DAQ.SignalingLed.led7, DAQ.SignalingLed.led8,
    --     }
    --     local colors = {
    --         DAQ.SignalingColors.blue, DAQ.SignalingColors.cyan,
    --         DAQ.SignalingColors.green, DAQ.SignalingColors.magenta, DAQ.SignalingColors.red,
    --         DAQ.SignalingColors.white, DAQ.SignalingColors.yellow
    --     }

    --     local atColor = 1
    --     local atLed = 1
    --     local popup = Popup("Signaling"):Text("Admire the pretty colors~")
    --     popup:Text("Press cancel to stop.")
    --     popup:Routine(function()
    --         daq:SignalingLedColor(leds[atLed], DAQ.SignalingColors.black)
    --         atLed = atLed + 1
    --         if atLed > 8 then
    --             atLed = 1
    --             atColor = atColor + 1
    --             if atColor > 7 then
    --                 atColor = 1
    --             end
    --         end
    --         daq:SignalingLedColor(leds[atLed], DAQ.SignalingColors.white)
    --         SleepFor(2)
    --     end)
    --     popup:Show()
    --     daq:SignalingMode(DAQ.SignalingModeEnum.standby)
    -- end)

    -- Test("Buzzer", function()
    --     ---@type DAQ
    --     local daq = Context.map.ibs.daq

    --     daq:SignalingBuzzerMode(DAQ.SignalingBuzzerPatternEnum.on125msPer250ms, 2)
    --     local popup = Popup("Beep Beep"):Text("Beep Beep")
    --     popup:Show()
    --     daq:SignalingBuzzerMode(DAQ.SignalingBuzzerPatternEnum.off)
    -- end)
end)
