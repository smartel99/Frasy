# Frasy - Generic Automated Hardware Testing Software
Frasy is a tool allowing technicians to test any Printed Circuit Board Assemblies (PCBA) in a highly scalable manner, ranging from small (less than 1000 units per year) to very large (> 10 million units per year) production lines.

Using a descriptive approach, very little programming knowledge are required in order to write test sequences. The technician only has to specify the criteria for a PCBA to be valid, and Frasy handles the rest!

**TODO: add screenshot of main gui**

## Example Sequence
This short example demonstrates how a voltage is measured on a PCBA.
```lua
local Commands = requires("commands.lua")

Sequence("MySequence", function(seqContext)
    Test("VCC", function(testContext)
        local tension = Commands.GetTension(testContext.Map.VCC)
        Expect(tension):ToBeBetween(3.0, 3.6)   -- Expects VCC to be 3.3V +/- 300mV.
    end)
end)
```

## What does Frasy do?
At its core, Frasy uses the functionalities of the [GoogleTest](https://github.com/google/googletest) unit testing framework to select, configure and execute sequences of tests defined by the user.
Tests can be individually controlled from within the software, and their behavior can be specified through scripts written in Lua.

When opening Frasy for the first time, the user will be prompted to select the files that contain the sequences of test. This will be remembered so that they are automatically loaded on start up in the future.

Sequences and tests can be enabled and disabled through the test explorer, which is also used to display the progression of the sequence. The user then only has to click on the "Run" button.

## Documentation
The API reference documentation can be found [here](doc/api/).

A thorough description of Frasy's architecture can be found [here](doc/architecture/)

## Thanks
Thank you to the following persons for their help in making this project possible!

- [DoubleNom](https://gitlab.com/DoubleNom) for the help designing the entire architecture and for his helpful criticism :)
- [nickclark2016](https://github.com/nickclark2016) for designing the test description procedure!

