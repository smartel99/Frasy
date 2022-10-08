# Expect
The `Expect` function allows conditions and expected values to be validated during a test.


```lua
function Test.Expect(self, value, name)
    -- Needs to be a future.
    assert(type(value) == "table")
    assert(type(value:IsReady) == "function")
    assert(type(value:WaitFor) == "function")
    assert(type(value:Get))

    local expectation = {
        -- Private members, shouldn't be accessible by the user.
        name = name,
        value = value,
        checkFunc = nil, -- Function to test the value.
        checkFuncArgs = nil,    -- Arguments to pass to the function.
        result = nil,   -- undefined till condition is tested.
        inverse = false,
        -- Applies the check function to the value and stores the result. Called by Frasy's back end, not exposed to the user.
        -- The manner in which this function handles the verification should depend on the type of `value`
        Check = function(self) 
            self.result = self.checkFunc(self.value, 
                type(self.checkFuncArgs) == "table" and table.unpack(self.checkFuncArgs) or self.checkFuncArgs)
            if self.inverse then self.result = not self.result end
            return self.result
        end,
        -- Inverses the logic applied to the condition.
        Not = function(self) end,
        -- Expects the value to be true or true-like. A function may be provided to determine the truthfullness of the value.
        ToBeTrue = function(self, func) end,
        -- Expects the value to be false or false-like.
        ToBeFalse = function(self) end,
        -- Expects the value to be within a range of acceptable values.
        ToBeNear = function(self, expected, variation) end,
        -- Expects the value to be equal to something, optionally using `pred` to verify the match.
        ToBeEqual = function(self, match, pred) end,
        ToBeAnyOf,
        ToBeType,
    }
end
```

## class `ExpectResult`


## `function ExportAs(self: ExpectResult, name: string)`
Makes the value, and results given to an `Expect` clause available to other tests and sequences.

**Parameters:**
- `name: string`: The name under which the value should be made available.

**Returns:** None

**Example:**
The following code:
```lua
Sequence("Foo", function(sequenceContext)
    Requires(Sequence():ToBeFirst)
    Test("FooTest", function(testContext)
        local voltage = 1.4
        Expect(voltage):ToBeNear(3.3, 0.1):ExportAs("Voltage")
    end)
end)

Sequence("Bar", function(sequenceContext)
    Test("BarTest", function(testContext)
        local results = Requires(Sequence("Foo"):Test("FooTest"):Value, "Voltage")
        print(string.format("Foo.FooTest.Voltage: %0.3f - %s", results.value, results.passed and "PASS" or "FAIL")
    end)
end)
```

Gives the following output:
```
Foo.FooTest.Voltage: 1.400 - FAIL
```