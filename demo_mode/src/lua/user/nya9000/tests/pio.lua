-- Sequence("PIO", function()
--     local pio = Context.map.ibs.pio
--     Test("Static Supply", function()
--         local voltages = {
--             [PIO.SupplyEnum.p3v3] = 3.3,
--             [PIO.SupplyEnum.p5v] = 5,
--             [PIO.SupplyEnum.p12v] = 12,
--             [PIO.SupplyEnum.p24v] = 24
--         }

--         for supply = PIO.SupplyEnum.p3v3, PIO.SupplyEnum.p24v do
--             pio:OutputEnable(supply, true)
--             SleepFor(100)
--             Expect(pio:Voltage(supply)):ToBeInPercentage(voltages[supply], 10)
--             Popup("Supply"):Text(PIO.SupplyEnumToOdName(supply) .. " is ON"):Show()
--             pio:OutputEnable(supply, false)
--             Popup("Supply"):Text(PIO.SupplyEnumToOdName(supply) .. " is OFF"):Show()
--         end
--     end)

--     Test("Variable Supply", function()
--         Requires(Test():ToBeFirst())
--         local voltages = {
--             5, 10, 15, 20
--         }
--         for supply = PIO.SupplyEnum.pVariable1, PIO.SupplyEnum.pVariable2 do
--             pio:OutputEnable(supply, true)

--             for index = 1, #voltages do
--                 local voltage = voltages[index]
--                 pio:DesiredVoltage(supply, voltage)
--                 SleepFor(100)
--                 Expect(pio:Voltage(supply)):ToBeInPercentage(voltage, 10)
--                 Popup("Supply"):Text(PIO.SupplyEnumToOdName(supply) .. " is ON @ " .. tostring(voltage) .. "V"):Show()
--             end

--             pio:OutputEnable(supply, false)
--             Popup("Supply"):Text(PIO.SupplyEnumToOdName(supply) .. " is OFF"):Show()
--         end
--     end)

--     Test("IOEXP", function()
--         pio:GpioConfigurations(0)
--         pio:GpioValues(0)

--         for i = 0, 11 do
--             pio:GpioValue(i, 1)
--             Popup("GPIO"):Text("IOEXP " .. tostring(i) .. " is HIGH"):Show()
--             pio:GpioValue(i, 0)
--         end
--     end)
-- end)
