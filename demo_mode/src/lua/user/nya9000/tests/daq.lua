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

    Test("IO Outputs", function()
        ---@type DAQ
        local daq = Context.map.ibs.daq

        daq:IoModes(0x1F)

        for _, io in ipairs(ios) do
            daq:IoValue(DAQ.IoEnum[io], DAQ.IoValueEnum.high)
            local popupHigh = Popup(io .. " High"):Text("Check that IO is high")
            popupHigh:Show()

            daq:IoValue(DAQ.IoEnum[io], DAQ.IoValueEnum.low)
            local popupLow = Popup(io .. " Low"):Text("Check that IO is Low")
            popupLow:Show()
        end
    end)
end)
