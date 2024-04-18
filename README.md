[![Documentation Status](https://readthedocs.org/projects/frasy/badge/?version=latest)](https://frasy.readthedocs.io/en/latest/?badge=latest)

# Frasy - Generic Automated Hardware Testing Software

![logo](doc/assets/img/frasy_logo_light.svg#gh-dark-mode-only)
![logo](doc/assets/img/frasy_logo.svg#gh-light-mode-only)

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

Sequences and tests can be enabled and disabled through the test explorer, which is also used to display the progression of the sequence. The user then only has to click on the "run" button.

## Documentation
The API reference documentation can be found [here](doc/API/).

A thorough description of Frasy's architecture can be found [here](doc/architecture/)

## Thanks
Thank you to the following persons for their help in making this project possible!

- [DoubleNom](https://gitlab.com/DoubleNom) for the help designing the entire architecture and for his helpful criticism :)
- [nickclark2016](https://github.com/nickclark2016) for designing the test description procedure!
- An anonymous sergal for the help coming up with sentences that makes sense!

## Getting Started
To generate the project files, use one of the scripts found [here](scripts/Windows/).
Alternatively, invoke Premake directly. 

**Note**: C++20 is required to build Frasy!

By default, Frasy builds the demonstration mode. The sources for this demo mode can be found [here](Frasy/src/demo_mode).
To provide your own source code, use the `--src_loc` flag when generating the project files.

## FAQ

### Why is the main branch called Morwenn?
Because [Morwenn](https://github.com/Morwenn).

### TODOs

- Automatic (re) connection to a USB device.
  - When the currently connected USB device disconnects, then reconnects, Frasy should automatically re-establish the connection.
  - When there are currently no USB device, and that a device with a VID/PID pair registered in the config is detected, automatically connect to it.
  - Current state: 
    - Connection is handled in Frasy/utils/communication/device, layer in Frasy/layers/device_viewer.*
    - Device notification hooks are setup with [Win32](https://learn.microsoft.com/en-us/windows/win32/devio/registering-for-device-notification) in Brigerad/src/Platform/Windows/WindowsWindow.*. They however are not working (no change is detected.)
    - The message handler should dispatch UsbConnected and UsbDisconnected events, with information on what the device is. (dispatch through Brigerad::Application::onEvent).
