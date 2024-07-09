Sequence("PIO", function()
    Test("ADC", function()
        ---@type DAQ
        local ib = Context.map.ibs.daq

        
        Log.D(""..#{DAQ.RoutingPointsEnum.P5V})
        local res = ib:MeasureVoltage({DAQ.RoutingPointsEnum.P5V}, DAQ.AdcChannelEnum.adc4)
        Log.I("Min: "..res.min)
        Log.I("Avg: "..res.average)
        Log.I("Max: "..res.max)
    end)
end)
