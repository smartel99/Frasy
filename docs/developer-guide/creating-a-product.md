# Creating a Product

A **product** is the unit of test configuration in Frasy. It bundles the environment declaration
(hardware setup, UUT count, execution policy) with the test sequences that validate a specific
PCBA. This page walks through creating one from scratch.

---

## Directory Layout

Frasy discovers products by scanning the `lua/user/` directory at runtime. Any subdirectory
containing an `environment.lua` file is treated as a product. The directory name becomes the
product name shown in the UI's product dropdown.

```
src/lua/user/
  <product-name>/
    environment.lua           ← required
    tests/
      *.lua                   ← one or more test files (loaded automatically)
```

!!! tip
    You can organize test files however you like — by phase (`power_on.lua`,
    `functional.lua`, `teardown.lua`), by board subsystem, or any other grouping that makes sense
    for your workflow. Frasy loads all `.lua` files found under `tests/` recursively.

---

## Step 1: Create the Directory

From your application root:

```
mkdir -p src/lua/user/my_board/tests
```

---

## Step 2: Write `environment.lua`

This file declares the static configuration of your test fixture. It must call
`Environment.Make()` with a configuration function.

### Minimal Environment (No Hardware)

If you just want to define logic-only tests (no instrumentation boards):

```lua
Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(1)
end)
```

### With Instrumentation Boards

If your fixture uses physical hardware boards for measurement and stimulus:

```lua
Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(1)
    Environment.Ib.Add(DAQ:New { name = "daq", nodeId = 2 })
    Environment.Ib.Add(PIO:New { name = "pio", nodeId = 3 })
end)
```

Once registered, they are accessible at runtime through `Context.map.ibs.<name>`.

### Multi-UUT Environment

For fixtures that test multiple boards in parallel:

```lua
Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(4)
    Environment.Ib.Add(DAQ:New { name = "daq", nodeId = 2 }) -- shared by all UUTs
    Environment.SetExecutionPolicy(ExecutionPolicy.parallel)
end)
```

### Mapping Test Points to Hardware Resources

Most test fixtures need to route logical test points on the UUT (e.g., "VCC", "SDA") to physical
hardware resources (e.g., a DAQ multiplexer channel). You can store this mapping directly in
`Context.values` so that test scripts reference named test points without hard-coding hardware
details.

A common pattern is to build a `route` table in `Context.values`:

```lua
Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(1)
    Environment.Ib.Add(DAQ:New { name = "daq", nodeId = 2 })

    Context.values.route = {
        vcc      = DAQ.RoutingPointsEnum.MUX1_A0,
        gnd      = DAQ.RoutingPointsEnum.MUX1_A1,
        sda      = DAQ.RoutingPointsEnum.MUX2_A0,
        scl      = DAQ.RoutingPointsEnum.MUX2_A1,
        led_out  = DAQ.RoutingPointsEnum.MUX3_A0,
    }
end)
```

In your tests, you then reference the named route instead of raw hardware constants:

```lua
Test("Supply Voltage", function()
    local daq = Context.map.ibs.daq --[[@as DAQ]]
    local v = daq:MeasureVoltage(Context.values.route.vcc)
    Expect(v.average, "VCC"):ToBeInPercentage(3.3, 5.0)
end)
```

This separation means that if a test point moves to a different mux channel (due to a fixture
revision), you only update the route table in `environment.lua` — no test files need to change.

You can also store non-routing values such as calibration constants, expected firmware versions,
or fixture-specific thresholds:

```lua
Context.values.expected_fw_version = "2.4.1"
Context.values.calibration = {
    voltage_offset = 0.012,
    current_gain   = 1.003,
}
```

### Environment API Reference

| Function | Purpose |
|---|---|
| `Environment.ScriptVersion(version)` | Script version string (appears in reports) |
| `Environment.Uut.Count(n)` | Number of UUTs tested in parallel |
| `Environment.Ib.Add(board)` | Register an instrumentation board |
| `Environment.SetExecutionPolicy(policy)` | `ExecutionPolicy.parallel` (default) or `ExecutionPolicy.sequential` |
| `Environment.Team.Add(leader, ...)` | Group UUTs into a team (for team-based synchronization). See [Teams](teams.md). |
| `Environment.UutValue.Add(key)` | Declare a per-UUT value (e.g., different test point routing per slot) |
| `Environment.SetOnReport(fn)` | Hook called on report generation to transform the report |
| `Environment.SetOnReportInfo(fn)` | Hook to inject extra metadata into reports |

---

## Step 3: Write Test Files

Create one or more `.lua` files in the `tests/` directory. Each file defines sequences and tests:

```lua
-- src/lua/user/my_board/tests/power_on.lua

Sequence("Power On", function()
    Requires(Sequence():ToBeFirst())

    Test("Supply Voltage", function()
        local daq = Context.map.ibs.daq --[[@as DAQ]]
        local v = daq:MeasureVoltage(Context.values.route.vcc)
        Expect(v.average, "VCC"):ToBeInPercentage(3.3, 5.0)
    end)

    Test("Current Draw", function()
        local daq = Context.map.ibs.daq --[[@as DAQ]]
        local i = daq:MeasureCurrent(Context.values.route.supply)
        Expect(i.average, "Idle Current"):ToBeLesser(0.100)
    end)
end)
```

```lua
-- src/lua/user/my_board/tests/functional.lua

Sequence("Functional", function()
    Requires(Sequence("Power On"):ToPass())

    Test("LED Toggle", function()
        local pio = Context.map.ibs.pio --[[@as PIO]]
        pio:SetOutput(1, true)
        SleepFor(100)
        local state = pio:ReadInput(2)
        Expect(state, "LED Feedback"):ToBeTrue()
    end)
end)
```

```lua
-- src/lua/user/my_board/tests/teardown.lua

Sequence("Teardown", function()
    Requires(Sequence():ToBeLast())

    Test("Power Off", function()
        local pio = Context.map.ibs.pio --[[@as PIO]]
        pio:SetOutput(1, false)
    end)
end)
```

---

## Step 4: Build and Run

Rebuild your application (or just let CMake's `refresh_lua_user` target sync the files):

```bat
cmake --build build
```

Launch the application. Your new product should appear in the product dropdown. Select it and
press **Run**.

!!! note
    If you modify Lua files while the application is running, press **F9** (or the "Reload"
    button) to re-scan products and reload the environment.

---

## Per-UUT Values

When testing multiple UUTs on the same fixture, each UUT might be wired to a different test point
route. Use `Environment.UutValue.Add()` to declare per-UUT constants:

```lua
local MyDaq = DAQ:New { name = "daq", nodeId = 2 }

Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(2)
    Environment.Ib.Add(MyDaq)

    Environment.UutValue.Add("vcc_route")
        :Link(1, DAQ.RoutingPointsEnum.MUX1_A0)
        :Link(2, DAQ.RoutingPointsEnum.MUX1_A1)
end)
```

In tests, access the value for the current UUT through `Context.values`:

```lua
Test("VCC", function()
    local route = Context.values.vcc_route
    local v = Context.map.ibs.daq:MeasureVoltage(route)
    Expect(v.average, "VCC"):ToBeInPercentage(3.3, 5.0)
end)
```

---

## Report Hooks

Frasy generates test reports in multiple formats (JSON, Key-Value, Markdown, PDF) after each run.
Two hooks let you inject extra information or transform the report before it is saved to disk.

### `Environment.SetOnReportInfo(fn)`

Called during report generation to inject extra metadata into the report header. The function
receives no arguments and must return a table of key-value pairs. These appear alongside the
built-in info fields (date, operator, serial, version, etc.) in all report formats.

**Use cases:**

- Identifying the physical fixture or station that produced the report
- Recording firmware versions read from the DUT during the test
- Tagging reports with a production lot or work order number
- Adding traceability fields required by your quality management system

```lua
Environment.SetOnReportInfo(function()
    return {
        fixture_id   = "FIX-001",
        station      = "Station A",
        work_order   = "WO-2026-1234",
        fw_version   = Context.values.detected_fw_version or "unknown",
    }
end)
```

In the generated JSON report, these appear as top-level fields in the `info` object. In
Key-Value reports, they are printed as `<key>: <value>` lines in the header section.

### `Environment.SetOnReport(fn)`

Called after the full report table has been assembled but **before** it is serialized to JSON on
disk. The function receives the complete report as a mutable table. You can add, remove, or
modify any field. The report is passed by reference — mutations apply directly.

**Use cases:**

- Stripping internal fields that should not appear in customer-facing reports
- Adding computed summaries (e.g., total pass rate, yield statistics)
- Renaming or restructuring fields to match an external system's expected schema
- Injecting a digital signature or checksum for tamper detection
- Filtering out expectations marked as debug-only

```lua
Environment.SetOnReport(function(report)
    -- Add a computed yield field
    local total = 0
    local passed = 0
    for _, seq in pairs(report.sequences) do
        for _, test in pairs(seq.tests or {}) do
            total = total + 1
            if test.pass then passed = passed + 1 end
        end
    end
    report.info.yield = string.format("%.1f%%", (passed / total) * 100)

    -- Remove a debug-only sequence from the report
    report.sequences["Debug Checks"] = nil
end)
```

!!! warning
    The report table is passed by reference. If your `onReport` function errors, the report is
    still saved in its current state (the error is logged but does not block report generation).
    Test your hook thoroughly to avoid corrupted reports.

---

## Checklist

Before running your product for the first time, verify:

- [ ] `src/lua/user/<name>/environment.lua` exists and calls `Environment.Make()`
- [ ] At least one test file exists in `src/lua/user/<name>/tests/`
- [ ] Each test file defines at least one `Sequence` containing at least one `Test`
- [ ] If using IBs, they are instantiated and registered with `Environment.Ib.Add()`
- [ ] EDS files are accessible at the paths declared by your boards
- [ ] The project builds without errors

---

## Next Steps

- [Teams](teams.md) — grouping UUTs for coordinated execution
- [Lua Setup](../getting-started/lua-setup.md) — environment and test scripting overview
- [Expectations](../lua-reference/expectations.md) — full list of matchers
- [Requirements](../lua-reference/requirements.md) — ordering and conditional execution
- [Custom Instrumentation Boards](custom-ibs.md) — defining your own board types
