Sequence("PIO", function()
    Test("ADC", function()
        ---@type DAQ
        local ib = Context.map.ibs.daq

        local res = ib:MeasureVoltage({DAQ.RoutingPointsEnum.P5V}, DAQ.AdcChannelEnum.adc2)
        Log.I("Min: "..res.min)
        Log.I("Avg: "..res.average)
        Log.I("Max: "..res.max)
    end)
end)
