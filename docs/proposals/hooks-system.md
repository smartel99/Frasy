# Proposal: Hooks System

**Status:** Draft  
**Date:** 2026-08-08

---

## Problem

Projects currently embed lifecycle logic (pre-test hardware checks, post-test LED signaling, report generation, product-specific setup) in C++ methods. This logic needs to move to Lua.

## Solution

A hooks system declared in `environment.lua` that allows products to inject Lua logic at well-defined points in the test lifecycle.

---

## Available Hooks

| Hook | Timing | Context | Returns |
|------|--------|---------|---------|
| `OnPreLaunch` | After environment loaded, plugins loaded, CANopen started. Before orchestrator starts | Global (no UUT context) | `true` to proceed, `false, "reason"` to abort |
| `OnPreSequence` | Before a sequence executes for a UUT | Per-UUT (receives `uut`, `sequence_name`) | void |
| `OnPostSequence` | After a sequence completes for a UUT | Per-UUT (receives `uut`, `sequence_name`, `passed`) | void |
| `OnPreTest` | Before a test executes for a UUT | Per-UUT (receives `uut`, `sequence_name`, `test_name`) | void |
| `OnPostTest` | After a test completes for a UUT | Per-UUT (receives `uut`, `sequence_name`, `test_name`, `passed`) | void |
| `OnComplete` | After all tests finish for a UUT | Per-UUT (receives `uut`, `results`) | void |
| `OnReport` | After built-in report generation | Per-UUT (receives `uut`, `result`) | `"suppress_default"` to skip built-in reports, or void |

---

## Declaration

```lua
-- In environment.lua
Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(2)
    Environment.Ib.Add(DAQ:New())
    Environment.Plugin.Load("visa")

    Environment.Hooks.OnPreLaunch(function()
        -- Power up testbench before running
        local pio = Context.map.ibs.pio
        pio:SetOutputEnable("P3V3", true)
        pio:SetOutputEnable("P12V", true)

        -- Verify hardware responds
        local hat = Context.map.ibs.hat
        local id = hat:Upload(0x1000, 0x00)
        if id ~= 100 then
            return false, "Device type mismatch: expected 100, got " .. tostring(id)
        end
        return true
    end)

    Environment.Hooks.OnPostTest(function(uut, sequence, test, passed)
        if not passed then
            Log.W("UUT " .. uut .. " failed: " .. sequence .. "/" .. test)
        end
    end)

    Environment.Hooks.OnComplete(function(uut, results)
        -- Set signaling LED
        local daq = Context.map.ibs.daq
        if results.passed then
            daq:DigitalWrite(0x6003, 0x01, 0x02)  -- green
        else
            daq:DigitalWrite(0x6003, 0x01, 0x04)  -- red
        end
    end)

    Environment.Hooks.OnReport(function(uut, result)
        -- Custom report generation on top of defaults
        MakeSmtReport(result)
    end)
end)
```

---

## Execution Order (Full)

```
1. Environment loaded
   - Plugins loaded (Environment.Plugin.Load)
   - CANopen nodes configured and started (from Environment.Ib.Add)
   - Implicit validation: declared plugins exist, declared nodes respond
2. Serial validation (Environment.Validation.Serial pattern)
3. Environment.Hooks.OnPreLaunch()
   → false, "reason" = abort with message
4. Orchestrator generates/validates solution
5. For each UUT (parallel per execution policy):
   For each sequence in solution:
     5a. Environment.Hooks.OnPreSequence(uut, sequence_name)
     5b. For each test in sequence:
         5b-i.   Environment.Hooks.OnPreTest(uut, sequence_name, test_name)
         5b-ii.  [Test executes — Expect(), measurements, etc.]
         5b-iii. Environment.Hooks.OnPostTest(uut, sequence_name, test_name, passed)
     5c. Environment.Hooks.OnPostSequence(uut, sequence_name, passed)
6. Environment.Hooks.OnComplete(uut, results)
7. Built-in report generation (per Environment.Report config)
8. Environment.Hooks.OnReport(uut, result)
```

---

## Hook Behavior Notes

- **OnPreLaunch** is the only hook that can abort the run. All others are informational/side-effectful.
- **Per-UUT hooks** run in the UUT's worker thread, so they have access to that UUT's `Context`.
- **OnReport** runs after built-in report generation. Return `"suppress_default"` to prevent the built-in reports from being written (for products that need completely custom report logic).
- Hooks are optional — if not declared, nothing happens at that point.
- Multiple declarations of the same hook are allowed (they execute in declaration order).
- All hooks work identically in GUI and headless modes.

---

## Reports Configuration

```lua
Environment.Make(function()
    -- Configure built-in report behavior
    Environment.Report.Formats({ "json", "kvp" })          -- which formats to produce
    Environment.Report.Directory("logs")                    -- output directory
    Environment.Report.Naming("{serial}_{result}_{date}")   -- filename template

    Environment.Hooks.OnReport(function(uut, result)
        -- Additional report actions (email, custom format, etc.)
    end)
end)
```
