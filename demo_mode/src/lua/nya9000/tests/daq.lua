Sequence("PIO", function()
    Test("ADC", function()
        local ib = Context.map.ibs.daq

        local res = ib:MeasureVoltage({ib.RoutingPointsEnum.P5V}, ib.AdcChannelEnum.adc4)
        Log.I("Min: "..res.min)
        Log.I("Avg: "..res.average)
        Log.I("Max: "..res.max)
    end)
end)
