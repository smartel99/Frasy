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
        Not = function(self) self.inverse = true; return self end,
        -- Expects the value to be true or true-like. A function may be provided to determine the truthfullness of the value.
        ToBeTrue = function(self, func) self.condition = function(v) return v == true end end,
        -- Expects the value to be false or false-like.
        ToBeFalse = function(self) self.condition = function(v) return v == false end end,
        -- Expects the value to be within a range of acceptable values.
        ToBeNear = function(self, expected, variation)
            self.checkFunc = function(v, exp, var)

            end
        end,
        -- Expects the value to be equal to something, optionally using `pred` to verify the match.
        ToBeEqual = function(self, match, pred) 
            if type(pred) == "function" then
                self.checkFunc = pred
            else
                self.checkFunc = function(val, expected) return val == expected end
            end
            self.checkFuncArgs = match
        end,
        ToBeAnyOf,
        -- Expects the value to be 
        ToBeType,
    }
    return 
end
```

## Example 1
```lua
Sequence("Seq", function(map, context)
    Test("Test", function()
        Expect(1).ToBeEqual(2)
    )
end)
```
