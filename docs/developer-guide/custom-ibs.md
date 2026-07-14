# Creating Custom Instrumentation Boards

!!! note "Coming Soon"
    This page is under construction.

This page will cover how to define your own instrumentation board types when the predefined
boards (`DAQ`, `PIO`, `R8L`) don't match your hardware.

## Topics to Cover

- Subclassing `Ib:New()` to create a custom board
- Providing an EDS file for your hardware's CANopen object dictionary
- Defining high-level helper methods (e.g., `MeasureVoltage`, `SetOutput`)
- Registering the board in `environment.lua`
