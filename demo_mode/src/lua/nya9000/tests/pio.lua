Sequence("PIO", function()
    Test("Var Supply", function()
        --local ib = Context.map.ibs.r8l
        --ib:Download(ib.od["Write Digital Output"], 0xFF)
        local ib = Context.map.ibs.pio
        --ib:Download(ib.od["Supply 3V3"]["Output Enable"], true)
        --ib:Download(ib.od["Supply 5V"]["Output Enable"], true)
        --ib:Download(ib.od["Supply 12V"]["Output Enable"], true)
        --ib:Download(ib.od["Supply 24V"]["Output Enable"], true)
        --Utils.sleep_for(5000)
        --ib:Download(ib.od["Supply 3V3"]["Output Enable"], false)
        --ib:Download(ib.od["Supply 5V"]["Output Enable"], false)
        --ib:Download(ib.od["Supply 12V"]["Output Enable"], false)
        --ib:Download(ib.od["Supply 24V"]["Output Enable"], false)

        local supply = "Variable Supply 2"
        local voltages = { 2.5, 20 }

        --for i = 0, 20 do
            ib:Download(ib.od[supply]["Output Enable"], true)
            for _, voltage in ipairs(voltages) do
                local label = string.format("%f", voltage)
                Log.d("Setting voltage to " .. voltage)
                ib:Download(ib.od[supply]["Desired Voltage"], voltage)
                Utils.sleep_for(1000)
                local actualVoltage = ib:Upload(ib.od[supply]["Voltage"])
                Log.d("Voltage read: " .. actualVoltage)
                Expect(actualVoltage, "Voltage " .. label):ToBeGreater(0.0)
                local current = ib:Upload(ib.od[supply]["Current"])
                Expect(current, "Current " .. label):ToBeGreater(0)
            end
        --end
        ib:Download(ib.od[supply]["Output Enable"], false)
    end)
end)
