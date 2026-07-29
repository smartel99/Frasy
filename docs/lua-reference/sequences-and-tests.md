# Sequences & Tests

This page documents the globals used to define and organize test logic: `Sequence()`, `Test()`,
and the multi-UUT coordination primitives `Sync()`, `Exclusive()`, and `Once()`.

---

## `Sequence(name, fn)`

Defines a named sequence — a group of related tests.

```lua
Sequence("Power On", function()
    -- Tests go here
end)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `name` | `string` | Unique name for this sequence (appears in reports and logs) |
| `fn` | `function` | Body of the sequence; must contain at least one `Test()` call |

**Rules:**

- Sequence names must be **unique** across all test files in a product.
- A sequence must contain at least one `Test()`.
- Sequences cannot be nested inside other sequences.
- The body function is called during generation to discover tests and requirements.

---

## `Sequence(name)` — Scope Getter

When called with a name only (no function), returns a **scope requirement** for use with
`Requires()`. When called with no arguments, returns the scope of the current sequence.

```lua
-- Reference the current sequence
Requires(Sequence():ToBeFirst())

-- Reference another sequence
Requires(Sequence("Power On"):ToPass())
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `name` | `string?` | Name of the sequence to reference. If `nil`, refers to the current sequence. |

**Returns:** A `ScopeRequirement` object with ordering and runtime methods (see
[Requirements](requirements.md)).

---

## `Test(name, fn)`

Defines a named test inside a sequence. This is where actual verification logic lives.

```lua
Sequence("Power On", function()
    Test("Supply Voltage", function()
        local v = readVoltage()
        Expect(v, "VCC"):ToBeInRange(4.9, 5.1)
    end)
end)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `name` | `string` | Unique name within the parent sequence (appears in reports and logs) |
| `fn` | `function` | Body of the test; performs measurements and assertions |

**Rules:**

- Test names must be unique **within their parent sequence** (different sequences can have
  tests with the same name).
- Tests must be defined inside a `Sequence()` body — not at the top level.
- Tests cannot be nested inside other tests.

---

## `Test(name)` — Scope Getter

When called with a name only (no function), returns a scope requirement for a test in the
**current** sequence. Combine with `Sequence()` to reference tests in other sequences.

```lua
-- Reference a test in the same sequence
Requires(Test("Basic Link"):ToPass())

-- Reference a test in another sequence
Requires(Sequence("Setup"):Test("Initialize"):ToPass())
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `name` | `string?` | Name of the test to reference. If `nil`, refers to the current test. |

**Returns:** A `ScopeRequirement` object.

---

## `Requires(requirement)`

Evaluates a requirement. If unmet, the current scope (test or sequence) is **skipped** — not
failed.

```lua
Requires(Sequence():ToBeFirst())
Requires(Test("Setup"):ToPass())
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `requirement` | `Requirement` | A requirement object (from scope getters or `RequirementSpecifier`) |

**Behavior:**

- During **generation**: order requirements influence the sort algorithm; runtime requirements
  always return `true`.
- During **validation**: all requirements return `true`.
- During **execution**: requirements are evaluated for real. If unmet, an `UnmetRequirement`
  error is raised, which the orchestrator catches and marks the scope as skipped.

See [Requirements](requirements.md) for the full requirement system.

---

## `Sync()`

Creates a **synchronization barrier** between sequences or tests. All UUTs must reach this
point before any can proceed past it.

```lua
Sequence("Phase 1", function()
    Test("Setup", function() ... end)
end)

Sync()  -- All UUTs must complete Phase 1 before Phase 2 starts

Sequence("Phase 2", function()
    Test("Measure", function() ... end)
end)
```

**Returns:** A `SyncRequirement` object (can be further configured, though this is rarely
needed).

**Where to call:**

- Between `Sequence()` definitions (at the top level of a test file) — creates a section
  boundary between sequences.

**How it works:**

- During generation, `Sync()` creates a section boundary in the Solution. Sequences before the
  barrier form one section; sequences after form the next.
- During execution, the orchestrator does not start the next section until all UUTs have
  completed the current one.

**Notes:**

- In single-UUT environments, `Sync()` has no practical effect (there's nothing to wait for).
- Within teams, `Team.Sync()` is called automatically at every test boundary — you don't need
  manual `Sync()` calls for team coordination.

---

## `Exclusive(id, fn)`

Ensures that only **one UUT at a time** can execute the protected function. Other UUTs
attempting to enter the same exclusive region will block until it's released.

```lua
Test("Shared Bus Access", function()
    Exclusive(1, function()
        -- Only one UUT executes this at a time
        writeToBus(data)
        local response = readFromBus()
        Expect(response, "Bus Response"):ToBeEqual(expected)
    end)
end)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `id` | `integer` | Mutex identifier. UUTs using the same ID will serialize access. |
| `fn` | `function` | The function to run under mutual exclusion |

**Returns:** The return value of `fn`.

**Notes:**

- Different IDs create independent mutexes — UUTs can enter `Exclusive(1, ...)` and
  `Exclusive(2, ...)` simultaneously.
- The lock is released when `fn` returns (or errors).
- Use `Exclusive` when multiple UUTs share a physical resource (e.g., a shared I²C bus, a
  single power supply channel).
- Store mutex IDs in `Context.values` for clarity:

```lua
-- In environment.lua
Context.values.mutex = { bus = 1, relay = 2 }

-- In tests
Exclusive(Context.values.mutex.bus, function()
    -- ...
end)
```

---

## `Once(fn)`

Executes a function **exactly once**, even if called concurrently from multiple UUTs. All UUTs
calling `Once()` at the same code location will see only a single execution; the others skip
it.

```lua
Test("Initialize Shared Resource", function()
    Once(function()
        -- Only one UUT actually runs this
        initializeSharedHardware()
    end)
    -- All UUTs continue here after the Once block
end)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `fn` | `function` | The function to execute once |

**Notes:**

- `Once()` uses the call site (stack trace + current scope) as a unique key — so the same
  `Once()` call in the same test will only execute once across all UUTs, but `Once()` calls at
  different locations are independent.
- Unlike `Exclusive()`, other UUTs don't wait — they simply skip the function if it has already
  been executed.
- Useful for one-time hardware initialization, shared calibration, or resource setup that
  should not be repeated per-UUT.

---

## Complete Example

```lua
-- tests/power_on.lua

Sequence("Power On", function()
    Requires(Sequence():ToBeFirst())

    Test("Enable Supply", function()
        Once(function()
            -- Only one UUT turns on the shared power supply
            Fixture.EnablePower()
            SleepFor(200)
        end)
    end)

    Test("Check Voltage", function()
        Requires(Test("Enable Supply"):ToPass())
        local v = readVoltage()
        Expect(v, "VCC"):ToBeInPercentage(3.3, 5.0)
    end)

    Test("Check Current", function()
        Requires(Test("Check Voltage"):ToPass())
        Exclusive(Context.values.mutex.global, function()
            local i = readCurrent()
            Expect(i, "Idle Current"):ToBeLesser(0.100)
        end)
    end)
end)

Sync()

Sequence("Functional", function()
    Test("Communication", function()
        local response = sendCommand(0x01)
        Expect(response, "ACK"):ToBeEqual(0x06)
    end)
end)

Sequence("Teardown", function()
    Requires(Sequence():ToBeLast())

    Test("Disable Supply", function()
        Once(function()
            Fixture.DisablePower()
        end)
    end)
end)
```

---

## Summary Table

| Global | Purpose |
|---|---|
| `Sequence(name, fn)` | Define a sequence |
| `Sequence(name?)` | Get a scope requirement for a sequence |
| `Test(name, fn)` | Define a test inside a sequence |
| `Test(name?)` | Get a scope requirement for a test |
| `Requires(req)` | Evaluate a requirement; skip scope if unmet |
| `Sync()` | Create a section barrier between sequences |
| `Exclusive(id, fn)` | Serialize access across UUTs (mutex) |
| `Once(fn)` | Execute a function exactly once across all UUTs |

---

## See Also

- [Requirements](requirements.md) — full ordering and runtime requirement system
- [Expectations](expectations.md) — `Expect()` and all matchers
- [Teams](../developer-guide/teams.md) — team-level synchronization (`Team.Tell`, `Team.Get`, etc.)
- [Test Lifecycle](../architecture/test-lifecycle.md) — how sequences are sorted, validated, and executed
