[![Documentation](https://img.shields.io/badge/docs-frasy.rald.ca-blue)](https://frasy.rald.ca)

![logo](docs/assets/frasy_logo_light.svg#gh-dark-mode-only)
![logo](docs/assets/frasy_logo.svg#gh-light-mode-only)

# Frasy — Generic Automated Hardware Testing Software

Frasy is a Windows desktop application for automated PCBA (Printed Circuit Board Assembly)
testing. It combines a C++ host process for UI, hardware communication, and test orchestration
with a Lua scripting layer where all test logic lives — allowing test engineers to write and
iterate on test sequences without recompiling the application.

## Quick Example

```lua
Sequence("Power On", function()
    Test("Check Supply Voltage", function()
        local daq = Context.map.ibs.daq --[[@as DAQ]]
        local v = daq:MeasureVoltage(Context.values.route.vcc)
        Expect(v.average, "Supply Voltage"):ToBeInPercentage(3.3, 5.0)
    end)
end)
```

## Key Features

- **Descriptive test scripting** — write what a board *should* do, not how to check it
- **Multi-UUT support** — test multiple boards in parallel with built-in synchronization
- **CANopen hardware integration** — communicate with instrumentation boards over SLCAN
- **Live UI panels** — log viewer, result viewer, statistical analyzer, CANopen browser, profiler
- **Hash-verified scripts** — integrity checking on all Lua files before execution
- **Test report generation** — JSON, Markdown, Key-Value, and PDF formats

## Documentation

Full documentation is available at **[frasy.rald.ca](https://frasy.rald.ca)**, including:

- [Getting Started](https://frasy.rald.ca/getting-started/) — installation, building, and first product setup
- [Architecture](https://frasy.rald.ca/architecture/) — how Frasy is structured
- [Developer Guide](https://frasy.rald.ca/developer-guide/) — customization and extension
- [Lua Reference](https://frasy.rald.ca/lua-reference/) — complete scripting API
- [Panels](https://frasy.rald.ca/panels/) — built-in UI panels reference

## Requirements

- Windows 10 or later
- C++23 compiler (MSVC / Visual Studio 2022 recommended)
- CMake 3.22+
- Git (for submodule checkout)

## Getting Started

Frasy is intended to be used as a git submodule in your application repository. See the the [installation guide](https://frasy.rald.ca/getting-started/installation/) to get started.

## License

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE) for
details.

## Acknowledgments

- [DoubleNom](https://gitlab.com/DoubleNom) — architecture design and invaluable criticism
- [nickclark2016](https://github.com/nickclark2016) — test description procedure design
