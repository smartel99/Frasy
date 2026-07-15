# Lua Layer

The Lua layer is where test engineers define what a board should do and how to verify it. Frasy embeds a Lua 5.4 runtime (via [sol2](https://github.com/ThePhD/sol2)) and provides a rich SDK of globals. The Lua files are loaded, validated, and executed by the C++ orchestrator without requiring a recompile of the application.

---

## Product Structure

A **product** is a directory under `lua/user/` that contains at least an `environment.lua` file:

```
lua/user/
  <product-name>/
    environment.lua         ← required: declares the test environment
    tests/
      *.lua                 ← one or more test files (any depth)
```

The application scans this directory tree at startup (and on ++f9++) to discover available products. The directory name becomes the product name shown in the UI.

!!! tip
    You can have multiple products side-by-side. The operator selects which one to run from a dropdown in the Control Room.

---

## Environment (`environment.lua`)

The environment file declares the **static configuration** of the test setup for this product — how many UUTs, which instrumentation boards are present, and how execution should proceed.

```lua
Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(2)
    Environment.Ib.Add(DAQ:New())
    Environment.SetExecutionPolicy(ExecutionPolicy.parallel)
end)
```

### Environment API

| Function | Purpose |
|---|---|
| `Environment.Make(fn)` | Wraps the environment definition; validates on completion |
| `Environment.ScriptVersion(version)` | Declares the script version string |
| `Environment.Uut.Count(n)` | Sets the number of UUTs (Units Under Test) |
| `Environment.Ib.Add(board)` | Registers an instrumentation board; parses its EDS file |
| `Environment.SetExecutionPolicy(policy)` | `ExecutionPolicy.parallel` or `ExecutionPolicy.sequential` |
| `Environment.UutValue.Add(key)` | Declares a per-UUT value with `.Link(uut, value)` chaining |
| `Environment.Team.Add(leader, ...)` | Groups UUTs into teams for coordinated execution |
| `Environment.Worker.Limit(n)` | Limits the number of concurrent worker threads |
| `Environment.SetOnReport(fn)` | Hook to transform the report before saving |
| `Environment.SetOnReportInfo(fn)` | Hook to inject extra metadata into reports |

---

## Sequences and Tests

Test files define **sequences** (groups of related tests) and the **tests** within them.

```lua
Sequence("Power On", function()
    Requires(Sequence():ToBeFirst())

    Test("Check Supply Voltage", function()
        local daq = Context.map.ibs.daq --[[@as DAQ]]
        local v = daq:MeasureVoltage(Context.values.route._24VDC)
        Expect(v, "Supply Voltage"):ToBeInPercentage(24.0, 5.0):Mandatory()
    end)

    Test("Check Current Draw", function()
        Requires(Test("Check Supply Voltage"):ToPass())
        local i = MeasureCurrent()
        Expect(i, "Current"):ToBeLesser(0.5)
    end)
end)
```

### Rules

- Sequence names must be **unique** across all test files for a product.
- Test names must be **unique within their sequence**.
- A sequence must contain at least one test.
- Test files are loaded in filesystem order; ordering between sequences is expressed via requirements, not file order.

---

## Requirements

Requirements express **constraints** between sequences and tests. They come in two flavors:

### Order Requirements (generation-time)

Influence how the orchestrator sorts sequences into the execution plan:

| Method | Effect |
|---|---|
| `Sequence():ToBeFirst()` | Place this sequence first |
| `Sequence():ToBeLast()` | Place this sequence last |
| `Sequence("Other"):ToBeBefore()` | Current runs before "Other" |
| `Sequence("Other"):ToBeAfter()` | Current runs after "Other" |

### Runtime Requirements (execution-time)

Evaluated live during execution; if unmet, the current scope is **skipped** (not failed):

| Method | Condition |
|---|---|
| `ToPass()` | Target scope completed and all expectations passed |
| `ToFail()` | Target scope did not pass |
| `ToBeComplete()` | Target scope ran (regardless of outcome) |

### Custom Requirements

```lua
Requires(RequirementSpecifier(function()
    return someCondition == true
end))
```

See [Test Lifecycle](test-lifecycle.md) for how requirements interact with the generation and execution stages.

---

## Expectations

Expectations are **assertions** on measured values. They are created with `Expect()` and chained with matchers:

```lua
Expect(value, "name"):ToBeInRange(min, max):Mandatory()
```

### Available Matchers

| Matcher | Assertion |
|---|---|
| `:ToBeTrue()` | Value is boolean `true` |
| `:ToBeFalse()` | Value is boolean `false` |
| `:ToBeEqual(expected)` | Value equals expected (type and value) |
| `:ToBeNear(expected, deviation)` | `expected ± deviation` |
| `:ToBeInRange(min, max)` | `min ≤ value ≤ max` |
| `:ToBeInPercentage(expected, pct)` | `expected ± (expected × pct/100)` |
| `:ToBeGreater(min)` | `value > min` |
| `:ToBeGreaterOrEqual(min)` | `value ≥ min` |
| `:ToBeLesser(max)` | `value < max` |
| `:ToBeLesserOrEqual(max)` | `value ≤ max` |
| `:ToBeType(typeName)` | `type(value) == typeName` |
| `:ToMatch(pattern)` | String matches Lua pattern |

### Modifiers

| Modifier | Effect |
|---|---|
| `:Mandatory()` | If this expectation fails, immediately abort the current test |
| `:Not()` | Inverts the assertion (e.g., `:Not():ToBeTrue()` asserts false) |
| `:ExportAs(name)` | Exports the value for use in later tests |
| `:OnErrorExtra(table)` | Attaches extra debug info to the result on failure |
| `:Show()` | Displays the expectation result in a popup |

---

## Multi-UUT Coordination

When testing multiple boards simultaneously (e.g., `Environment.Uut.Count(2)`), each UUT runs through the same test scripts in its own thread. Most of the time they execute independently and in parallel. However, real test fixtures often have **shared resources** (a single power supply, a shared communication bus, a calibration reference) or require **coordinated timing** (applying a load to all boards at the same instant).

Frasy provides three synchronization primitives to handle these cases:

### `Sync()`

Inserts a **barrier** — all enabled UUTs must reach the same `Sync()` point before any of them continues.

**When to use it:** When you need all boards to perform an action at the same time — for example, simultaneously applying a load to verify that a shared power rail doesn't sag, or ensuring all boards are in a known state before a cross-talk measurement.

```lua
Test("Simultaneous Load", function()
    ApplyLoad()       -- each UUT prepares independently
    Sync()              -- everyone waits here until all UUTs have applied their load

    local i = MeasurePowerSupplyCurrent()
    Expect(i, "Current"):ToBeInRange(1.0, 1.2)
end)
```

```mermaid
sequenceDiagram
    participant UUT1
    participant UUT2
    participant UUT3

    UUT1->>UUT1: ApplyLoad()
    UUT2->>UUT2: ApplyLoad()
    UUT3->>UUT3: ApplyLoad()
    UUT1-->>UUT1: reaches Sync()
    Note over UUT1: waiting...
    UUT3-->>UUT3: reaches Sync()
    Note over UUT3: waiting...
    UUT2-->>UUT2: reaches Sync()
    Note over UUT1,UUT3: barrier met — all continue
    UUT1->>UUT1: MeasurePowerSupplyCurrent()
    UUT2->>UUT2: MeasurePowerSupplyCurrent()
    UUT3->>UUT3: MeasurePowerSupplyCurrent()
```

### `Exclusive(id, fn)`

A **mutex** — only one UUT at a time may execute the protected function. Other UUTs wait their turn.

**When to use it:** When multiple UUTs need to access a shared physical resource that can't handle concurrent access — for example, a single programmable power supply, a shared I²C bus, or a calibration instrument that serves all slots.

The `id` parameter identifies *which* mutex to use. Different IDs are independent mutexes, so you can protect different resources without unnecessary blocking.

```lua
Exclusive(1, function()
    -- Only one UUT at a time can talk to the shared power supply
    sharedPowerSupply:setVoltage(5.0)
    sharedPowerSupply:waitStable()
end)

Exclusive(2, function()
    -- A different mutex for a different shared resource
    sharedMultimeter:measure()
end)
```

### `Once(fn)`

Executes a function **exactly once** across all UUTs, regardless of how many call it. The first UUT to reach the call runs it; all others skip it.

**When to use it:** For one-time setup actions that affect all UUTs equally — calibrating a shared fixture, initializing a shared instrument, or loading a reference dataset that all boards will use.

`Once` is scoped per call-site — it uses a hash of the call's traceback and the current scope to identify uniqueness. This means two different `Once()` calls in different tests will each run once independently.

```lua
Once(function()
    -- Runs exactly once, even with 4 UUTs hitting this point
    calibrateSharedFixture()
end)
```

!!! tip "Sharing values between UUTs"
    UUTs can also share data with each other at runtime (e.g., a calibration value computed by one UUT and needed by others). See the [Developer Guide](../developer-guide/index.md) for patterns and examples.

---

## Context

The `Context` global provides runtime information accessible from any test script:

```lua
Context.info.uut          -- UUT index (1-based)
Context.info.serial       -- Serial number string
Context.info.operator     -- Operator name
Context.info.stage        -- Current stage (Stage.generation/validation/execution)
Context.info.time         -- Timing info: start, stop, elapsed, process
Context.info.orchestrator -- { version, date }
Context.info.user         -- { version, date } (from Environment.ScriptVersion)

Context.map.ibs           -- Registered instrumentation boards
Context.map.uuts          -- UUT indices
Context.map.values        -- Per-UUT values declared in environment

Context.values            -- Runtime value store (per-sequence namespace)
Context.values.gui        -- Values injected from C++ (setLoadUserValues)
```

---

## Logging

```lua
Log.D("debug message")     -- Debug level
Log.I("info message")      -- Info level
Log.W("warning message")   -- Warning level
Log.E("error message")     -- Error level
Log.T("trace message")     -- Trace level
```

Log messages appear in the Log Window panel with the UUT index as context.

---

## Popups

Operator-facing dialogs built in Lua and rendered by the C++ UI:

```lua
Popup("Confirm Fixture")
    :Text("Place the board in the fixture")
    :Text("Then press OK")
    :Button("OK", function() end)
    :Show()
```

The test execution **pauses** until the operator dismisses the popup. Popups support text, dynamic text, inputs, buttons, images, and layout directives (horizontal/vertical grouping, springs, same-line).

---

## Instrumentation Boards (Ibs)

Boards registered via `Environment.Ib.Add()` become available as `Ibs.<name>`:

```lua
local board = Ibs.MyBoard.ib

-- Read a value from the board's object dictionary
local voltage = board:Upload(board.od["Supply Voltage"])

-- Write a value to the board
board:Download(board.od["DAC Output"], 2.5)

-- Utility methods
board:Reset()
board:Serial()
board:SoftwareVersion()
board:HardwareVersion()
```

The `od` table is automatically populated by parsing the board's EDS file. See [Hardware Communication](hardware.md) for details on how this maps to CANopen SDO operations.

---

## Core SDK File Layout

```
Frasy/lua/core/
  framework/
    orchestrator.lua        ← Orchestrator logic (sequences, tests, execution)
    expectation/            ← Expectation matchers (generation, validation, execution variants)
    scope.lua               ← Scope definition
    scope_requirement/      ← ScopeRequirement methods per stage
    runtime_requirement.lua ← RuntimeRequirement wrapper
    order_requirement.lua   ← Order requirement registration
    sync_requirement.lua    ← Sync barrier implementation
    context.lua             ← Context global definition
    stage.lua               ← Stage enum (idle, generation, validation, execution)
    exception.lua           ← Error types (UnmetRequirement, UnmetExpectation, etc.)
    error_handler.lua       ← xpcall error handler with traceback
  sdk/
    test.lua                ← Sequence(), Test(), Expect(), Requires(), Sync(), etc.
    log.lua                 ← Log.D/I/W/E/T stubs (replaced by C++ at runtime)
    popup.lua               ← Popup builder
    environment/            ← Environment.Make and sub-APIs
  utils/                    ← Helper functions (type checks, bitwise, ini parser, etc.)
  can_open/                 ← CANopen OD parser, type definitions
  cep/                      ← Board-specific SDKs (DAQ, PIO, R8L)
  vendor/                   ← Third-party Lua libraries (json.lua)
```
