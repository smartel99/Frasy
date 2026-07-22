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
local MyDaq = DAQ:New({ name = "daq", nodeId = 2 })
local MyPio = PIO:New({ name = "pio", nodeId = 3 })

Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(1)
    Environment.Ib.Add(MyDaq)
    Environment.Ib.Add(MyPio)
end)
```

The boards are instantiated **outside** `Environment.Make()` so they can be `require`d from test
files if needed. Once registered, they are accessible at runtime through `Context.map.ibs.<name>`.

### Multi-UUT Environment

For fixtures that test multiple boards in parallel:

```lua
local MyDaq = DAQ:New({ name = "daq", nodeId = 2 })

Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(4)
    Environment.Ib.Add(MyDaq)
    Environment.SetExecutionPolicy(ExecutionPolicy.parallel)
end)
```

### Environment API Reference

| Function | Purpose |
|---|---|
| `Environment.ScriptVersion(version)` | Script version string (appears in reports) |
| `Environment.Uut.Count(n)` | Number of UUTs tested in parallel |
| `Environment.Ib.Add(board)` | Register an instrumentation board |
| `Environment.SetExecutionPolicy(policy)` | `ExecutionPolicy.parallel` (default) or `ExecutionPolicy.sequential` |
| `Environment.Team.Add(leader, ...)` | Group UUTs into a team (for team-based synchronization) |
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
local MyDaq = DAQ:New({ name = "daq", nodeId = 2 })

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

You can customize test reports by providing hook functions in the environment:

```lua
Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(1)

    Environment.SetOnReportInfo(function()
        return {
            fixture_id = "FIX-001",
            station = "Station A",
        }
    end)

    Environment.SetOnReport(function(report)
        -- Transform the report before it's saved
        report.custom_field = "custom value"
        return report
    end)
end)
```

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

- [Lua Setup](../getting-started/lua-setup.md) — environment and test scripting overview
- [Expectations](../lua-reference/expectations.md) — full list of matchers
- [Requirements](../lua-reference/requirements.md) — ordering and conditional execution
- [Custom Instrumentation Boards](custom-ibs.md) — defining your own board types
