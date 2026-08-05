--- @file    test.lua
--- @brief   CLI-tests-multi — validates popup serialization across multiple UUTs.

Sequence("Multi-UUT Popup", function()
    Test("Per-UUT Input", function()
        local receivedSerial = nil
        Popup("Enter Serial")
            :Text("Enter serial number for UUT " .. Context.info.uut)
            :Input("Serial")
            :Button("OK", function(inputs)
                receivedSerial = inputs[1]
            end, { consume = true })
            :Show()
        -- Each UUT should receive its own serial from stdin
        Expect(receivedSerial, "Serial Received"):Not():ToBeEqual("")
        Expect(type(receivedSerial), "Serial Is String"):ToBeEqual("string")
    end)

    Test("Basic Check", function()
        Expect(Context.info.uut, "UUT Index"):ToBeGreater(0)
        Expect(true, "Always Pass"):ToBeTrue()
    end)
end)
