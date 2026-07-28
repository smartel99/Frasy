# Environment

The `Environment` global provides all functions for configuring a product's test setup. It is
called exclusively from `environment.lua` — the required file in every product directory.

---

## `Environment.Make(fn)`

Wraps the entire environment declaration. Must be called exactly once per `environment.lua`.
Validates team assignments and worker configuration on completion.

```lua
Environment.Make(function()
    -- All other Environment.* calls go here
end)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `fn` | `function` | Configuration function containing all environment declarations |

---

## `Environment.ScriptVersion(version)`

Declares the script version string. This appears in test reports for traceability.

```lua
Environment.ScriptVersion("1.0.0")
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `version` | `string` or `number` | Version identifier (converted to string internally) |

**Returns:** When called with no argument, returns the current script version string.

---

## `Environment.Uut.Count(n)`

Sets the number of Units Under Test that this fixture tests in parallel.

```lua
Environment.Uut.Count(4)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `n` | `integer` | Number of UUTs (must be ≥ 1) |

**Notes:**

- For single-UUT setups, use `Environment.Uut.Count(1)`.
- Multi-UUT setups enable concurrency primitives (`Sync()`, `Exclusive()`, `Once()`).
- Each UUT gets its own execution thread during the execution stage.

---

## `Environment.Ib.Add(board)`

Registers a CANopen instrumentation board. The framework parses the board's EDS file and
builds its object dictionary.

```lua
local MyDaq = DAQ:New({ name = "daq", nodeId = 2 })
Environment.Ib.Add(MyDaq)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `board` | table | A board instance (must have a `.ib` field with `name`, `nodeId`, and `eds`) |

**Returns:** The board instance (allows chaining).

**Notes:**

- Board names must be unique. Registering a duplicate name raises an error.
- After registration, the board is accessible at `Context.map.ibs.<name>`.
- The EDS file is parsed immediately and the object dictionary is attached as `board.ib.od`.
- Predefined board types: `DAQ`, `PIO`, `R8L`.
- For non-CANopen devices, add them directly to `Context.map.ibs` instead (see
  [Custom Instrumentation Boards](../developer-guide/custom-ibs.md)).

---

## `Environment.SetExecutionPolicy(policy)`

Controls whether sequences run in parallel or sequentially across UUTs.

```lua
Environment.SetExecutionPolicy(ExecutionPolicy.parallel)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `policy` | `ExecutionPolicyEnum` | `ExecutionPolicy.parallel` (default) or `ExecutionPolicy.sequential` |

**Notes:**

- `parallel` — UUTs execute concurrently within each section. This is the default.
- `sequential` — UUTs execute one at a time. Useful when all UUTs share a single hardware
  resource that cannot be accessed concurrently.
- For single-UUT environments, this setting has no effect.

### `ExecutionPolicy` Enum

```lua
ExecutionPolicy = {
    parallel   = 0,
    sequential = 1,
}
```

---

## `Environment.Team.Add(leader, ...)`

Groups UUTs into a team for coordinated execution. See [Teams](../developer-guide/teams.md)
for full documentation.

```lua
Environment.Team.Add(1, 2)    -- UUT 1 leads UUT 2
Environment.Team.Add(3, 4)    -- UUT 3 leads UUT 4
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `leader` | `integer` | UUT number of the team leader (position 1) |
| `...` | `integer...` | UUT numbers of the followers (positions 2, 3, ...) |

**Rules:**

- If any `Team.Add()` call is made, **all** UUTs must be assigned to a team.
- A UUT cannot belong to multiple teams.
- The first argument is always the leader.

---

## `Environment.UutValue.Add(key)`

Declares a per-UUT value — a constant that differs for each UUT slot. Returns a builder
with a `:Link()` method to assign values.

```lua
Environment.UutValue.Add("vcc_route")
    :Link(1, DAQ.RoutingPointsEnum.MUX1_A0)
    :Link(2, DAQ.RoutingPointsEnum.MUX1_A1)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `key` | `string` | Name of the value (accessible in tests as `Context.values.<key>`) |

**Returns:** A builder object with a `:Link(uut, value)` method.

### `:Link(uut, value)`

Assigns a value to a specific UUT for this key.

| Name | Type | Description |
|---|---|---|
| `uut` | `integer` | UUT number (1-indexed) |
| `value` | any | The value for this UUT |

**Returns:** The builder (chainable).

**Usage in tests:**

```lua
Test("VCC", function()
    local route = Context.values.vcc_route  -- automatically resolves to the current UUT's value
    local v = Context.map.ibs.daq:MeasureVoltage(route)
    Expect(v.average, "VCC"):ToBeInPercentage(3.3, 5.0)
end)
```

---

## `Environment.Worker.Limit(specifier)`

Limits how many UUTs can run concurrently based on shared resource constraints. Returns a
builder to configure the limit.

```lua
Environment.Worker.Limit(Ib):To(2, Team)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `specifier` | `Ib` | The type of resource to limit by (currently only `Ib` is supported) |

**Returns:** A builder object with a `:To(count, reference)` method.

### `:To(count, reference)`

| Name | Type | Description |
|---|---|---|
| `count` | `integer` | Maximum number of concurrent groups sharing the resource |
| `reference` | `Team` | The grouping unit (currently only `Team` is supported) |

**Notes:**

- Only one `Worker.Limit` call is allowed per environment.
- This is an advanced feature for fixtures where multiple teams share the same physical IB
  and cannot all communicate simultaneously.

---

## `Environment.SetOnReport(fn)`

Registers a hook called after the report table is assembled but before it is serialized to
disk. Use it to transform, filter, or augment the report.

```lua
Environment.SetOnReport(function(report)
    -- Add computed yield
    local total, passed = 0, 0
    for _, seq in pairs(report.sequences) do
        for _, test in pairs(seq.tests or {}) do
            total = total + 1
            if test.pass then passed = passed + 1 end
        end
    end
    report.info.yield = string.format("%.1f%%", (passed / total) * 100)
end)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `fn` | `function(report)` | Receives the full report table (mutable). Modifications apply directly. |

**Notes:**

- The report is passed by reference — mutate it in place.
- If the function errors, the report is saved in its current state (the error is logged but
  does not block report generation).

---

## `Environment.SetOnReportInfo(fn)`

Registers a hook called during report generation to inject extra metadata into the report
header.

```lua
Environment.SetOnReportInfo(function()
    return {
        fixture_id = "FIX-001",
        station    = "Station A",
        work_order = "WO-2026-1234",
    }
end)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `fn` | `function() → table` | Must return a table of key-value pairs to add to the report info section |

**Notes:**

- Keys appear alongside built-in info fields (date, operator, serial, version, etc.) in all
  report formats.
- Called once per UUT per test run.

---

## Complete Example

```lua
local MyDaq = DAQ:New({ name = "daq", nodeId = 2 })
local MyPio = PIO:New({ nodeId = 5 })

Environment.Make(function()
    Environment.ScriptVersion("2.1.0")
    Environment.Uut.Count(2)
    Environment.SetExecutionPolicy(ExecutionPolicy.parallel)

    -- Hardware
    Environment.Ib.Add(MyDaq)
    Environment.Ib.Add(MyPio)

    -- Teams
    Environment.Team.Add(1, 2)

    -- Per-UUT routing
    Environment.UutValue.Add("vcc_route")
        :Link(1, DAQ.RoutingPointsEnum.MUX1_A0)
        :Link(2, DAQ.RoutingPointsEnum.MUX1_A1)

    -- Shared values
    Context.values.route = {
        gnd = DAQ.RoutingPointsEnum.MUX2_A0,
    }
    Context.values.expected_fw = "3.2.1"

    -- Report hooks
    Environment.SetOnReportInfo(function()
        return { fixture_id = "FIX-042" }
    end)

    Environment.SetOnReport(function(report)
        report.sequences["Debug"] = nil  -- Strip debug sequence from reports
    end)
end)
```

---

## See Also

- [Creating a Product](../developer-guide/creating-a-product.md) — full walkthrough of setting up a product
- [Teams](../developer-guide/teams.md) — team system documentation
- [Custom Instrumentation Boards](../developer-guide/custom-ibs.md) — defining custom board types
- [Context](context.md) — runtime access to environment data from test scripts
