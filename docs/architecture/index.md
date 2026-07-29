# Architecture

This section describes how Frasy is structured internally — from the C++ host process down to the Lua scripting layer and hardware communication.

Frasy follows a layered architecture where your application extends thin framework classes, the framework handles orchestration and UI, and Lua scripts define the actual test logic. This separation keeps test development fast (no recompilation) while the C++ layer provides performance and hardware access.

## Pages

- [Overview](overview.md) — high-level block diagram, layer responsibilities, and data flow
- [C++ Layer](cpp-layer.md) — the application host, class hierarchy, panels, and override points
- [Lua Layer](lua-layer.md) — product structure, environment, sequences, expectations, and the core SDK
- [Hardware Communication](hardware.md) — CANopen over SLCAN, EDS files, object dictionary, and SDO operations
- [Test Lifecycle](test-lifecycle.md) — generation, validation, execution stages, UUT states, and the Solution model
