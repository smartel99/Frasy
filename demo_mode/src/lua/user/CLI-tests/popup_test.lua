--- @file    popup_test.lua
--- @brief   CLI-tests — popup interaction test for headless mode validation.

Sequence("Popup", function()
    Requires(Sequence("Basic"):ToPass())

    Test("Confirm Popup", function()
        Popup("Confirm Action")
            :Text("Please confirm to continue.")
            :Text("This is a headless popup test.")
            :Button("OK", function() end, { consume = true })
            :Show()
        Expect(true, "Popup Consumed"):ToBeTrue()
    end)

    Test("Single Input Popup", function()
        Requires(Test("Confirm Popup"):ToPass())
        local receivedValue = nil
        Popup("Enter Value")
            :Text("Please provide a value below:")
            :Input("Test Value")
            :Button("Submit", function(inputs)
                receivedValue = inputs[1]
            end, { consume = true })
            :Show()
        Expect(receivedValue, "Received Input"):ToBeEqual("hello123")
    end)

    Test("Multiple Inputs Popup", function()
        Requires(Test("Single Input Popup"):ToPass())
        local lot = nil
        local operator_id = nil
        local batch = nil
        Popup("Multi Input")
            :Text("Enter all fields:")
            :Input("Lot Number")
            :Input("Operator ID")
            :Input("Batch Code")
            :Button("Confirm", function(inputs)
                lot = inputs[1]
                operator_id = inputs[2]
                batch = inputs[3]
            end, { consume = true })
            :Show()
        Expect(lot, "Lot Number"):ToBeEqual("LOT-001")
        Expect(operator_id, "Operator ID"):ToBeEqual("OP-42")
        Expect(batch, "Batch Code"):ToBeEqual("BATCH-X")
    end)
end)
