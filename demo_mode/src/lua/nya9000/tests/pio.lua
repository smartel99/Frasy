Sequence("PIO", function()
    Test("Fixed Supply", function()
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

        local supply = "Variable Supply 1"
        local voltages = { 0.0, 2.5, 5, 7.5, 10, 12.5, 15, 17.5, 20 }
        local enables = { false, true }
        for _, enable in ipairs(enables) do
            ib:Download(ib.od[supply]["Output Enable"], enable)
            for __, voltage in ipairs(voltages) do
                ib:Download(ib.od[supply]["Desired Voltage"], voltage)
                Utils.sleep_for(10)
                local actualVoltage = ib:Upload(ib.od[supply]["Voltage"])
                Expect(actualVoltage, "Voltage"):ToBeGreater(0.0)
                local current = ib:Upload(ib.od[supply]["Current"])
                Expect(current, "Current"):ToBeGreater(0)
            end
        end
    end)
end)
