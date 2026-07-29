# Context

`Context` is a global table available in all Lua scripts. It holds runtime information about
the current test execution — which UUT is running, what stage the orchestrator is in, what
hardware is available, and any values declared in the environment.

---

## Structure Overview

!!! warning "Do not modify Context during test execution"
    `Context` is managed by the framework. Modifying anything other than `Context.values` during
    test execution can cause unpredictable behavior, corrupted reports, or crashes. Treat
    `Context.info`, `Context.map`, `Context.team`, and `Context.orchestrator` as **read-only**
    from within test scripts.

```lua
Context = {
    info = {
        stage    = Stage.execution,     -- current orchestrator stage
        uut      = 1,                   -- current UUT number (1-indexed)
        operator = "John",              -- operator name (set at run time)
        serial   = "SN123456789",       -- serial number for this UUT
        title    = "My Product",        -- product title
        version  = {
            frasy        = "1.2.3-42",  -- Frasy framework version
            application  = "1.0.0",     -- your application version
            orchestrator = "1.2.0",     -- orchestrator version
            scripts      = "1.0.0",     -- script version (from Environment.ScriptVersion)
        },
    },
    map = {
        uuts     = {1, 2, ...},         -- list of UUT numbers
        ibs      = { daq = ..., ... },  -- registered instrumentation boards
        values   = { ... },             -- per-UUT value declarations
        onReport     = function(report) end,  -- report transform hook
        onReportInfo = function() return {} end,  -- report info hook
    },
    values = {
        -- user-defined values from environment.lua
        route = { ... },
        gui   = { ... },                -- values from setLoadUserValues callback
    },
    team = {
        hasTeam = false,                -- whether teams are enabled
        players = { ... },              -- UUT → {leader, position} mapping
        teams   = { ... },              -- leader → {members} mapping
    },
    orchestrator = {
        scope     = Scope,              -- current sequence/test being executed
        sequences = { ... },            -- all registered sequences and their tests
        solution  = { ... },            -- the execution plan
        values    = { ... },            -- exported values from expectations
    },
}
```

---

## `Context.info`

Runtime metadata about the current execution. Read-only from test scripts.

### `Context.info.stage`

The current orchestrator stage. Compare against the `Stage` enum:

```lua
if Context.info.stage == Stage.execution then
    -- Only do hardware I/O during execution
    local v = daq:MeasureVoltage(channel)
end
```

### `Context.info.uut`

The 1-indexed UUT number for the current execution thread. In multi-UUT environments, each
UUT runs in its own thread and sees its own `uut` value.

```lua
Log.I("Running on UUT " .. Context.info.uut)
```

### `Context.info.operator`

The operator name provided when the test run was started (from the Control Room input field).

```lua
Log.I("Operator: " .. Context.info.operator)
```

### `Context.info.serial`

The serial number assigned to the current UUT for this run.

```lua
Log.I("Testing board: " .. Context.info.serial)
```

### `Context.info.title`

The product title (typically the product directory name).

### `Context.info.version`

A table of version strings for traceability in reports:

| Key | Description |
|---|---|
| `frasy` | Frasy framework version (e.g., `"1.2.3-42"`) |
| `application` | Your application version (set via C++) |
| `orchestrator` | Orchestrator version |
| `scripts` | Script version (from `Environment.ScriptVersion()`) |

---

## `Context.map`

The environment map — hardware and configuration declared in `environment.lua`.

### `Context.map.ibs`

Table of registered instrumentation boards, keyed by name:

```lua
local daq = Context.map.ibs.daq --[[@as DAQ]]
local v = daq:MeasureVoltage(channel)
```

This includes:

- Boards registered with `Environment.Ib.Add()` (CANopen boards)
- Boards added directly via `Context.map.ibs["name"] = ...` (non-CANopen devices)

### `Context.map.uuts`

Array of UUT numbers (`{1, 2, 3, ...}`). Its length equals `Environment.Uut.Count(n)`.

```lua
local uutCount = #Context.map.uuts
```

### `Context.map.values`

The raw per-UUT value declarations from `Environment.UutValue.Add()`. This is the internal
storage — for convenience, per-UUT values are also resolved into `Context.values` during
execution (see below).

### `Context.map.onReport` / `Context.map.onReportInfo`

The report hooks registered via `Environment.SetOnReport()` and `Environment.SetOnReportInfo()`.
These are called by the framework — you typically don't invoke them directly.

---

## `Context.values`

User-defined values from `environment.lua`. This is where you store test point routes,
calibration constants, thresholds, and any other configuration that test scripts need.

```lua
-- Set in environment.lua:
Context.values.route = {
    vcc = DAQ.RoutingPointsEnum.MUX1_A0,
    gnd = DAQ.RoutingPointsEnum.MUX2_A0,
}
Context.values.expected_fw = "3.2.1"

-- Read in tests:
local v = daq:MeasureVoltage(Context.values.route.vcc)
```

### `Context.values.gui`

Values injected from the C++ side via `setLoadUserValues()`. Typically contains operator inputs
or fixture-specific configuration:

```lua
local fixtureId = Context.values.gui.fixture_id
```

### Per-UUT Values

Values declared with `Environment.UutValue.Add()` are stored in `Context.map.values` and
resolved per-UUT at runtime. Each UUT sees its own linked value:

```lua
-- Declared in environment.lua:
Environment.UutValue.Add("vcc_route")
    :Link(1, DAQ.RoutingPointsEnum.MUX1_A0)
    :Link(2, DAQ.RoutingPointsEnum.MUX1_A1)

-- In tests, each UUT gets its linked value:
local route = Context.values.vcc_route
```

---

## `Context.team`

Team configuration (only populated when `Environment.Team.Add()` is used).

### `Context.team.hasTeam`

Boolean indicating whether teams are enabled for this environment.

```lua
if Context.team.hasTeam then
    -- Team coordination logic
end
```

### `Context.team.players`

Maps each UUT number to its team info:

```lua
Context.team.players[1] = { leader = 1, position = 1 }
Context.team.players[2] = { leader = 1, position = 2 }
```

### `Context.team.teams`

Maps each leader UUT number to the list of team members:

```lua
Context.team.teams[1] = { 1, 2 }  -- UUT 1 leads UUT 2
```

!!! tip
    Use the `Team.*` API functions (`Team.IsLeader()`, `Team.Position()`, etc.) rather than
    reading `Context.team` directly. The API is safer and more readable.

---

## `Context.orchestrator`

Internal orchestrator state. Mostly used by the framework, but some fields are useful in
advanced scenarios.

### `Context.orchestrator.scope`

The current `Scope` object — identifies which sequence and test is executing:

```lua
local currentSequence = Context.orchestrator.scope.sequence
local currentTest     = Context.orchestrator.scope.test
```

### `Context.orchestrator.sequences`

The full registry of sequences and their tests. Used internally by the framework for
requirement resolution and result tracking.

### `Context.orchestrator.values`

Stored values exported via `Expect(...):ExportAs(name)`. Keyed by sequence → test → name.
Use `Sequence("..."):Test("..."):Value("name")` to retrieve them safely.

---

## `Stage` Enum

The `Stage` global enum identifies the orchestrator's current phase:

```lua
Stage = {
    idle       = 0,  -- not running
    generation = 1,  -- discovering sequences and building the Solution
    validation = 2,  -- verifying script integrity
    execution  = 3,  -- running tests for real
}
```

Common usage:

```lua
-- Guard hardware calls
if Context.info.stage ~= Stage.execution then return end

-- Check stage in custom functions
if Context.info.stage == Stage.generation then
    -- Return dummy data
end
```

---

## Complete Example

```lua
Sequence("Diagnostics", function()
    Test("Report Context", function()
        Log.I("Stage: " .. Context.info.stage)
        Log.I("UUT: " .. Context.info.uut)
        Log.I("Operator: " .. Context.info.operator)
        Log.I("Serial: " .. Context.info.serial)
        Log.I("Frasy Version: " .. Context.info.version.frasy)
        Log.I("Script Version: " .. Context.info.version.scripts)

        -- Access hardware
        local daq = Context.map.ibs.daq --[[@as DAQ]]
        local v = daq:MeasureVoltage(Context.values.route.vcc)
        Expect(v.average, "VCC"):ToBeInPercentage(3.3, 5.0)

        -- Access GUI values
        if Context.values.gui and Context.values.gui.debug then
            Log.W("Running in debug mode")
        end
    end)
end)
```

---

## See Also

- [Environment](environment.md) — declaring `Context.map` and `Context.values`
- [Teams](../developer-guide/teams.md) — team-related `Context.team` usage
- [Test Lifecycle](../architecture/test-lifecycle.md) — how stages progress
- [Exposing C++ to Lua](../developer-guide/cpp-to-lua.md) — `Context.values.gui` from `setLoadUserValues`
