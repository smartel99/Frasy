Sequence("DAQ", function()
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
    --     for i, mode in pairs(DAQ.SignalingModeEnum) do
    --         daq:SignalingMode(mode)
    --         local popup = Popup("Singaling"):Text("Admire the pretty colors~")
    --         popup:Show()
    --     end
    --     daq:SignalingMode(DAQ.SignalingModeEnum.standby)
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
    --         Utils.SleepFor(2)
    --     end)
    --     popup:Show()
    --     daq:SignalingMode(DAQ.SignalingModeEnum.standby)
    -- end)

    Test("Buzzer", function()
        ---@type DAQ
        local daq = Context.map.ibs.daq

        daq:SignalingBuzzerMode(DAQ.SignalingBuzzerPatternEnum.on125msPer250ms, 2)
        local popup = Popup("Beep Beep"):Text("Beep Beep")
        popup:Show()
        daq:SignalingBuzzerMode(DAQ.SignalingBuzzerPatternEnum.off)
    end)
end)
