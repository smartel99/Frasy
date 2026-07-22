# Lua Setup

This page covers how to create a product — the Lua side of a Frasy application. A product
defines the test environment and the test sequences that run against your hardware.

---

## Product Directory Structure

Frasy discovers products by scanning the `lua/user/` directory at runtime. Each subdirectory
that contains an `environment.lua` file is treated as a product. The directory name becomes the
product name shown in the UI.

```
src/lua/user/
  <product-name>/
    environment.lua       ← required: declares the test environment
    tests/
      *.lua               ← one or more test files
```

For example, a product called "demo":

```
src/lua/user/
  demo/
    environment.lua
    tests/
      power_on.lua
      functional.lua
```

All `.lua` files under `tests/` are loaded automatically. You can organize them however you like
— by test phase, by board section, etc.

---

## Environment Configuration

`environment.lua` declares the static configuration of your test setup: how many UUTs are tested,
what hardware (instrumentation boards) the fixture uses, and the mapping between access points on
the UUT and the test hardware. It must call `Environment.Make()` with a function that configures
the environment.

### Minimal Environment

The simplest environment declares a script version and a single UUT:

```lua
Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(1)
end)
```

### Environment API

| Function | Purpose |
|---|---|
| `Environment.ScriptVersion(version)` | Set the script version string (for traceability) |
| `Environment.Uut.Count(n)` | Declare how many UUTs are tested in parallel |
| `Environment.Ib.Add(board)` | Register an instrumentation board |
| `Environment.SetExecutionPolicy(policy)` | `ExecutionPolicy.parallel` or `ExecutionPolicy.sequential` |

### Adding Instrumentation Boards

If your test fixture uses hardware boards (IBs) for measurement and control, declare them in the
environment. Frasy provides predefined board types (like `DAQ`, `PIO`, `R8L`) that handle node
IDs and EDS files automatically for existing SMarTest devices:

```lua
local MyDaq = DAQ:New({ name = "my_daq", nodeId = 2 })

Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(1)
    Environment.Ib.Add(MyDaq)
end)
```

Once registered, the board is accessible in tests through `Context.ibs.my_daq`.

If the predefined board types don't cover your hardware, you can define your own custom
instrumentation boards. See [Creating Custom Instrumentation Boards](../developer-guide/custom-ibs.md).

### Execution Policy

By default, sequences run in parallel across UUTs. For single-UUT setups this has no effect. If
you need strictly sequential execution:

```lua
Environment.SetExecutionPolicy(ExecutionPolicy.sequential)
```

---

## Writing Test Sequences

Test files define sequences and the tests within them. A sequence groups related tests, and each
test contains the actual verification logic.

### Basic Structure

```lua
Sequence("Power On", function()
    Test("Check Supply Voltage", function()
        local voltage = readVoltage()  -- your measurement function
        Expect(voltage, "Supply Voltage"):ToBeInRange(4.9, 5.1)
    end)

    Test("Check Current Draw", function()
        local current = readCurrent()
        Expect(current, "Idle Current"):ToBeLesser(0.5)
    end)
end)
```

### `Sequence(name, function)`

Defines a named sequence. The name must be unique across all test files in the product. The
function body should contain one or more `Test()` calls.

### `Test(name, function)`

Defines a named test inside a sequence. The name must be unique within its parent sequence. The
function body performs measurements and makes assertions using `Expect()`.

---

## Expectations

`Expect(value, name)` creates an expectation. The `value` is what you measured, and `name` is a
human-readable label that appears in test reports. Chain a matcher to assert the value:

| Matcher | Description |
|---|---|
| `:ToBeTrue()` | Value is `true` |
| `:ToBeFalse()` | Value is `false` |
| `:ToBeEqual(expected)` | Value equals `expected` |
| `:ToBeNear(expected, deviation)` | Value is within ±`deviation` of `expected` |
| `:ToBeInRange(min, max)` | Value is between `min` and `max` (inclusive) |
| `:ToBeInPercentage(expected, pct)` | Value is within ±`pct`% of `expected` |
| `:ToBeGreater(min)` | Value is strictly greater than `min` |
| `:ToBeGreaterOrEqual(min)` | Value is greater than or equal to `min` |
| `:ToBeLesser(max)` | Value is strictly less than `max` |
| `:ToBeLesserOrEqual(max)` | Value is less than or equal to `max` |
| `:ToBeType(type)` | Value is of Lua type `type` (e.g., `"number"`, `"string"`) |
| `:ToMatch(pattern)` | String value matches Lua pattern |

### Modifiers

| Modifier | Description |
|---|---|
| `:Not()` | Inverts the expectation (e.g., `:Not():ToBeEqual(0)`) |
| `:Mandatory()` | If this expectation fails, skip the rest of the test immediately |

Modifiers are chainable and can appear before the matcher:

```lua
Expect(status, "Device Status"):Not():ToBeEqual(0)
Expect(voltage, "Critical Voltage"):Mandatory():ToBeInRange(3.0, 3.6)
```

---

## Ordering with Requirements

Sequences and tests have no guaranteed execution order by default — they are stored in a hash
map, so insertion order is not preserved. Use `Requires()` to express explicit ordering or
conditional execution.

### Ordering Examples

```lua
Sequence("Setup", function()
    Requires(Sequence():ToBeFirst())  -- always runs first
    Test("Initialize", function()
        -- ...
    end)
end)

Sequence("Teardown", function()
    Requires(Sequence():ToBeLast())  -- always runs last
    Test("Power Off", function()
        -- ...
    end)
end)
```

### Conditional Execution

Skip a sequence or test based on the result of another:

```lua
Sequence("Calibration", function()
    Requires(Sequence("Setup"):ToPass())  -- skip if Setup failed

    Test("Verify Offset", function()
        -- ...
    end)
end)
```

Within a sequence, skip a test if a previous one failed:

```lua
Sequence("Communication", function()
    Test("Basic Link", function()
        -- ...
    end)

    Test("Data Transfer", function()
        Requires(Test("Basic Link"):ToPass())  -- skip if Basic Link failed
        -- ...
    end)
end)
```

For the full requirements system, see [Requirements](../lua-reference/requirements.md).

---

## Logging

Use the `Log` global to write messages to the application's log panel:

```lua
Log.D("Debug message")     -- debug level
Log.I("Info message")      -- info level
Log.W("Warning message")   -- warning level
Log.E("Error message")     -- error level
```

---

## Complete Example

Here is a minimal but complete product:

**`src/lua/user/my_product/environment.lua`**

```lua
Environment.Make(function()
    Environment.ScriptVersion("1.0.0")
    Environment.Uut.Count(1)
end)
```

**`src/lua/user/my_product/tests/basic.lua`**

```lua
Sequence("Self Test", function()
    Requires(Sequence():ToBeFirst())

    Test("Always Passes", function()
        Expect(true, "Sanity Check"):ToBeTrue()
    end)

    Test("Math Works", function()
        Expect(2 + 2, "Addition"):ToBeEqual(4)
        Expect(10.0, "Range Check"):ToBeInRange(9.5, 10.5)
    end)
end)
```

This product will appear as "my_product" in the UI dropdown and can be run immediately.

---

## Multi-UUT Setups

If your environment declares more than one UUT (`Environment.Uut.Count(2)` or more), Frasy runs
test sequences concurrently across all UUTs. This introduces coordination primitives like
`Sync()` and `Exclusive()` for synchronization and mutual exclusion.

For details on multi-UUT coordination, see the [Lua Reference](../lua-reference/index.md).
For grouping UUTs that share resources, see [Teams](../developer-guide/teams.md).

---

## Next Steps

- [Expectations](../lua-reference/expectations.md) — full matcher reference
- [Requirements](../lua-reference/requirements.md) — complete ordering and runtime requirement system
