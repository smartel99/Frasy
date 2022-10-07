# Use Case 1
Verifies the impedances of the power rails before programming the UUT, then reads a tension.

The order of execution must be:
1. Verify the impedances
2. Program the UUT
3. Read the tension

The impedances can be executed out of order.

## sequence.lua
This file contains the description of the conditions that needs to be met by the board for a PASS status.
```lua
local Testbench = require "testbench.lua"

Sequence("Test Sequence", function(context)
    Test("Impedances", function(ctx)
        local impedance = Testbench.GetImpedance(params)
        -- Allow 10% of deviation, injecting 1V.
        Expect(impedance).ToBeNear(params.Expected, params.Tolerance)
    end, {
        -- Expects R1 to be 1k +/-10%.
        -- Expects R2 to be 100k +/- 1%.
        {
            R1 = { ImpP = map.TP1, ImpN = map.TP2, Expected = 1000, Tolerance = 0.10, Guards = {TP7, TP6}, InputVoltage = 0.1, SettlingTime = 0.1 },
            R2 = { ImpP = map.TP2, ImpN = map.TP3, Expected = 100000, Tolerance = 0.01, Guards = {TP8}, InputVoltage = 0.2, SettlingTime = 0.01 },
        }
    }):Requires(Test():DoFirst())

    Test("Program UUT", function(ctx)
        local result = Testbench.ProgramUUT()
        Expect(result).ToBeTrue()
    end):Requires(Test("Impedances").ToPass(), State.Power.IsOn())

    Test("Read Tension", function(ctx)
        local tension = Testbench.GetTension(map.TP1)
        -- Expects 1V +/- 1%.
        Expect(tension).ToBeNear(1.0, 0.01)
    end):Requires(Test("Impedances"):ToPass(),
                  Test("Program UUT"):ToBeCompleted())
end)
```

## testbench.lua
This file contains the functions that are specific for the test bench, interfacing with the HAL. It also contains the mapping of abstract, project-specific signals to their real-life counterpart.

```lua
local testbench = {}

local Signals = require "signal.lua"
local Commands = require "commands.lua"

function testbench.GetImpedance(params)
    if params.InputFrequency == nil or params.InputFrequency == 0 or params.InputShape == nil or params.InputShape == "DC" then
        params.InputFrequency = 0
        params.InputShape = "DC"
    end
    
    return Commands.GetImpedance(
                                 params.ImpP.connectedTo,   -- Injection point.
                                 params.ImpN.connectedTo,   -- Reading point.
                                 params.Guards,             -- Guard channels.
                                 params.Expected,           -- Expected value (for auto-ranging).
                                 params.InputVoltage,       -- Tension injected.
                                 params.InputFrequency,     -- Frequency of the signal injected.
                                 params.InputShape,         -- Shape of the signal injected (DC, sine, square, triangle).
                                 params.SettlingTime        -- Time between injection and acquisition.
                                )
end

return testbench
```

## commands.lua
This file contains the generic, all-purpose functions offered by the daughter board.

```lua
local commands = {}

function commands.GetImpedance(impP, impN, guards, expected, inVolt, inFreq, inShape, settlingTime)
    -- Get samples to take from the frequency.
    local samplesToTake = inShape == "DC" and 5 or ComputeSamplesToTakeForCylces(frequency, 4)
    local range = HAL.ImpMeter.RangeFromExpected(expected)

    local orders = HAL.NewOrderList()
        :AddParameter(tension, "tension")       -- Adds "tension" as a parameter. The value of tension will be forwarded to the `Then` function.
        :AddParameter(frequency, "frequency")   -- Adds "frequency" as a parameter.

    -- First inject the tension on the test point.
    orders:AddOrder(HAL.NewOrder(HAL.Generator)
                        :SelectPoint(signal)        -- Select the injection point.
                        :SetFrequency(frequency)    -- Signal's frequency.
                        :SetTension(tension)        -- Signal's tension.
                        :SetShape(shape)            -- Signal's shape.
                        :SetRange(rangeId)          -- Resistor range to use.
                    )

    -- Then measure the output signal.
    orders:AddOrder(HAL.NewOrder(HAL.DVM)
                        :SelectPoint(signal)        -- Select the mesuring point.
                        :SetSamplingFrequency(32000) -- Sample at 32ksps.
                        :SyncWith(HAL.Generator)    -- Synchronize acquisition with the generator.
                        :TakeSamples(samplesToTake)
                        :AddParameter(range, "range")        -- Pass range with the key "range" to OnGetResult
                        :PassResultThrough("raw")   -- Pass the measured value to `OnGetResult` through the "raw" key.
                        :OnGetResult(function(values) return ScaleToRange(values["raw"], values["range"]) end)
                    ):PassResultThrough("measured")   -- Pass the value of the order (fetched by calling its `OnGetResult` function) to the list's `OnGetResult`.

    orders:OnGetResult(function(values)
        return AFunctionThatCalculatesTheImpedance(values["measured"], values["tension"], values["frequency"])
    end)

    -- Send the list of order.
    return HAL.SendOrderList(orders)
end

return commands
```

## Expected Output
```json
{
    // information about frasy, versions, serial nubmers, etc...
    "Impedances":{
        "Result": "PASS",
        "Tests": [
            {
                "Logic": {
                    "ToBeNear": {
                        "Target": 1000,
                        "AllowedVariation": "10%",
                    }
                },
                "Parameters": {
                     "Name": "R1",
                     "ImpP": "TP1", 
                     "ImpN": "TP2", 
                     "Expected": 1000,
                     "Tolerance": 0.10,
                     "Guards": ["TP7", "TP6"],
                     "InputVoltage": 0.1,
                     "SettlingTime": 0.1 
                },
                "Result": "PASS",
                "Value": 1000,
            },
            {
                "Logic": {
                    "ToBeNear": {
                        "Target": 100000,
                        "AllowedVariation": "1%",
                    }
                },
                "Parameters": {
                    "Name": "R2",
                    "ImpP": "TP2",
                    "ImpN": "TP3",
                    "Expected": 100000,
                    "Tolerance": 0.01,
                    "Guards": ["TP8"],
                    "InputVoltage": 0.2,
                    "SettlingTime": 0.01
                },
                "Result": "PASS",
                "Value": 100000,
            }
        ]
    },
    "Program UUT": {
        "Logic": "ToBeTrue",
        "Requires": [
            {
                "Test": {
                    "Name": "Impedances",
                    "Condition": "ToPass"
                }
            }
        ],
        "Result": "PASS",
        "Value": true
    },
    "Read Tension": {
        "Logic": {
            "ToBeNear": {
                "Target": 1.0,
                "AllowedVariation": "10%"
            }
        },
        "Requires": [
            {
                "Test": {
                    "Name": "Impedance",
                    "Condition": "ToPass"
                }
            },
            {
                "Test": {
                    "Name": "Program UUT",
                    "Condition": "ToBeCompleted"
                }
            }
        ],
        "Result": "PASS",
        "Value": 1.0
    }
}
```
