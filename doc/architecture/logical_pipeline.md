# Pipeline of Commands from Lua to Test Report
This page describes the sequence of events that happens between the test script and the generation of results at the end.

This pipeline has multiple layers, split into Lua, C++, Frasy and Generic Daughterboard(TM) hardware.

1. [Test Layer](#test-layer) [Frasy/Lua]
2. [Presentation Layer](#presentation-layer) [Frasy/Lua]
3. [Command Layer](#command-layer) [Frasy/Lua]
4. [Hardware-Abstraction Layer](#hardware-abstraction-layer) [Frasy/Lua]
5. [Data Link Layer](#data-link-layer) [Frasy/C++]
6. [Physical Layer](#physical-layer) [Generic Daughterboard/C++]

In this pipeline, most layers are generic and therefore don't have to be modified at all on a per-project basis. However, layer 1 (Test) and layer 2 (Presentation) needs to be implemented for each specific projects due to their nature.

## Test Layer
The test layer corresponds to the highest level of abstraction, where the sequence of a test is described in a manner that is unique to a specific application.

This layer contains one or many individual tests that can be executed in any order.

They must be written in a way that does not require a specific order of execution. 

## Presentation Layer
This layer is where the project-specific functionalities are defined.

### Mapping
The Mapping table is in charge of mapping the abstracted labels used by the layers above to their physical counterpart. This applies to both the daughter board on which the signal is located, and the physical pin on which that signal is.

The mapping table is what will be used by the HAL to do the correlation between the requested abstract name and the physical hardware. All valid signals must thus be defined by the user.

Each entry in the table must contain two fields:
- DaughterboardId: The ID of the daughterboard on which the signal is located.
- SignalId: The ID of the signal to use for that abstract label.

Example:
```lua
Mapping = {
TP1 = {DaughterboardId = 0, SignalId = Signals.U1_A0 }
}
```

Details can be found [here](mapping.md).

## Command Layer
This is the layer where every basic actions possible to be done by the daughter boards are defined.

These actions are rudimentary and should be used as building blocs by the presentation layer.

Actions are based on the Noun-Verb paradigm, where a noun is used to execute a specific action.

This allows for multiple verbs to be stacked efficiently back to back for a single noun. 

For example, the command `DMM.Select("TP1").ReadDCTension()` uses the verbs `Select` and `ReadTension` on the noun `DMM`.

The details of this paradigm as well as a list of every noun and their associated verbs can be found [here](nouns.md).

## Hardware-Abstraction Layer
This is the layer where the Noun-Verb commands are translated into lists of specific instructions for the hardware to execute.

For example, `DMM.Select("TP1").ReadDCTension()` will be translated into 
```lua
TestPoint.Select(Mapping["TP1"])    -- Selects the signal corresponding to TP1.
Tools.DMM.ReadDCTension(Mapping["TP1"]) -- Select the DMM that can be used by TP1.
```

The details of the HAL can be found [here](hal.md).


## Data Link Layer
This is the layer where the exchange of packets through COM ports is handled. 

It is in charge of dispatching commands received by the transport layer to the right COM port, as well as handling the reception of responses from the physical layers.

## Physical Layer
This is the layer where things physically happen, where the operations are accomplished and where the data is acquired.

There might be one or multiple instances of this layer, each one of these instances being connected to the Data Link layer through a unique, individual COM port.

The physical is responsible for the validation of the commands, both in their construction and their validity in the current context. It is thus necessary to keep track of the hardware's state, i.e. which channels are currently active, which bus is in use and which is free, etc.

**Put simply, no actions should be taken unless necessary.**

In the case where a resource is not available (no free bus, tool in use, etc.), the list of commands should be held in a queue until the required resources become available.


# Example
This example shows the entire pipeline of a very simple project.
It is broken down into the individual files that constitutes each layer.

### test_layer.lua (Project-Specific)
A test named `PowerSupply` is defined, where a resistor labelled `R32` is expected to have a value that is equal to 1 kilohms +/- 10%. The 5 kilohms range will be used to measure that value.

```lua
local Fixture = require "presentation_layer.lua"

test(UUT, PowerSupply, function()
    EXPECT_NEAR(Fixture.ResistorValue("R32", "5k"), 1000.0, 100.0)
end)
```

### presentation_layer.lua (Project-Specific)
The high-level functions are defined in this file.
Here, `ResistorValue` is defined as a function that uses the HAL to get the value of a resistance.

To measure the resistance, a tension must be injected in the component and its tension measured on the other side of the resistor. It is thus needed for this function to have an internal mapping of the resistors and their injection/measuring points.

```lua
local presentation = {}
local Commands = requires "command_layer.lua"
local Mapping = requires "mapping.lua"

local resistorMapping = {
    R32 = {
        InjectOn = Mapping.TP1,
        ReadFrom = Mapping.TP2
    }
}

local resistorRanges = {
    "100" = 0,
    "5k" = 1,
    "100k" = 2,
    "1M" = 3
}

function presentation.ResistorValue(resistor, range)
    -- Make sure that the resistor is valid first...
    local tps = resistorMapping[resistor]
    if tps == nil then return error("Invalid resistor requested!") end
    local rangeId = resistorRanges[range]
    if rangeId == nil then return error("Invalid resistor range requested") end

    -- Inject the signal in the UUT.
    -- Create a new order for the signal generator.
    local order = HAL.NewOrder(HAL.Generator)
    order:SelectPoint(tps.InjectOn)  -- Select the injection point.
    order:SetFrequency(0)            -- DC signal.
    order:SetTension(1)              -- Signal's tension = 1V
    order:SetRange(rangeId)          -- Resistor range to use.

    -- Send the order to the generator.
    HAL.SendOrder(order)

    -- Read the tension
end

return presentation
```