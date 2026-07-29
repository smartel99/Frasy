# Requirements

Requirements express **ordering constraints** and **conditional execution** between sequences
and tests. They are passed to `Requires()` which either silently passes or skips the current
scope.

---

## How Requirements Work

```lua
Requires(Sequence("Setup"):ToPass())
```

1. `Sequence("Setup")` returns a **scope requirement** object pointing at the "Setup" sequence.
2. `:ToPass()` creates a **runtime requirement** that checks whether "Setup" passed.
3. `Requires()` evaluates the requirement:
    - If met → execution continues normally.
    - If unmet → the current scope is **skipped** (not failed).

### Stage Behavior

| Stage | Order Requirements | Runtime Requirements |
|---|---|---|
| **Generation** | Collected and fed to the sort algorithm | Always return `true` |
| **Validation** | Ignored (sort already done) | Always return `true` |
| **Execution** | Ignored (sort already done) | Evaluated for real |

This means ordering is determined at generation time, and conditional execution is enforced at
runtime.

---

## Scope Requirements

Scope requirements are obtained by calling `Sequence()` or `Test()` as getters (without a
function body). They provide both **order methods** and **runtime methods**.

### Getting a Scope

```lua
-- Current sequence
Sequence()

-- A named sequence
Sequence("Power On")

-- Current test
Test()

-- A named test in the current sequence
Test("Check Voltage")

-- A test in another sequence
Sequence("Power On"):Test("Check Voltage")
```

### `:Test(name)`

Narrows the scope to a specific test within the referenced sequence.

```lua
Sequence("Setup"):Test("Initialize"):ToPass()
```

### `:Value(name)`

Retrieves an exported value from the referenced scope (set via `Expect(...):ExportAs(name)`).

```lua
local refVoltage = Sequence("Calibration"):Test("Reference"):Value("ref_voltage")
```

---

## Order Requirements

Order requirements influence how the sort algorithm arranges sequences and tests during
generation. They only affect **ordering**, not whether something runs.

### `:ToBeFirst()`

Places the current scope at the beginning of the execution plan.

```lua
Sequence("Setup", function()
    Requires(Sequence():ToBeFirst())
    -- ...
end)
```

### `:ToBeLast()`

Places the current scope at the end of the execution plan.

```lua
Sequence("Teardown", function()
    Requires(Sequence():ToBeLast())
    -- ...
end)
```

### `:ToBeBefore()`

The **referenced** scope must run before the **current** scope.

```lua
Sequence("B", function()
    -- "A" must run before "B"
    Requires(Sequence("A"):ToBeBefore())
    -- ...
end)
```

!!! note
    `:ToBeBefore()` means "the thing I'm pointing at should be before me." It creates a
    dependency from the current scope onto the referenced scope.

### `:ToBeAfter()`

The **referenced** scope must run after the **current** scope.

```lua
Sequence("A", function()
    -- "B" must run after "A"
    Requires(Sequence("B"):ToBeAfter())
    -- ...
end)
```

### `:ToBeRightBefore()`

Like `:ToBeBefore()` but implies direct adjacency (no other scope between them) when possible.

```lua
Requires(Sequence("Calibration"):ToBeRightBefore())
```

### `:ToBeRightAfter()`

Like `:ToBeAfter()` but implies direct adjacency.

```lua
Requires(Sequence("Verification"):ToBeRightAfter())
```

### Forward References

Order requirements support **forward references** — you can reference sequences or tests that
haven't been defined yet. The generation stage retries unresolved references until all are
satisfied or no progress can be made.

---

## Runtime Requirements

Runtime requirements are evaluated during **execution only**. They determine whether a scope
actually runs based on the results of previously-executed scopes.

### `:ToPass()`

The scope runs only if the referenced scope **passed** (all expectations met).

```lua
Sequence("Functional", function()
    Requires(Sequence("Power On"):ToPass())
    -- Skipped if "Power On" failed
end)

Test("Data Transfer", function()
    Requires(Test("Basic Link"):ToPass())
    -- Skipped if "Basic Link" failed
end)
```

This is the most common runtime requirement. It also implicitly creates an ordering constraint
(the referenced scope must run before the current one).

### `:ToFail()`

The scope runs only if the referenced scope **failed**.

```lua
Sequence("Error Recovery", function()
    Requires(Sequence("Main Test"):ToFail())
    -- Only runs if "Main Test" failed
end)
```

### `:ToBeComplete()`

The scope runs only if the referenced scope was **not skipped** (it ran to completion,
regardless of pass/fail).

```lua
Test("Cleanup", function()
    Requires(Test("Dangerous Operation"):ToBeComplete())
    -- Skipped if "Dangerous Operation" was itself skipped
end)
```

### `:HasPassed()` / `:HasFailed()`

Direct boolean queries (not wrapped in `Requires()`). Useful for branching logic within a test
body:

```lua
Test("Conditional Check", function()
    if Sequence("Optional"):HasPassed() then
        -- Do extra verification
    end
end)
```

---

## Custom Requirements with `RequirementSpecifier`

For requirements that don't fit the scope-based model, create a custom runtime requirement with
`RequirementSpecifier()`:

```lua
Requires(RequirementSpecifier(function()
    return os.clock() < 300  -- Only run within first 5 minutes
end))
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `fn` | `function() → boolean` | Must return `true` (met) or `false` (unmet) |

**Returns:** A `RuntimeRequirement` object compatible with `Requires()`.

!!! warning
    Custom requirement functions are evaluated during **execution only**. During generation and
    validation they are not called (scope requirements always return `true` in those stages).

---

## Combining Requirements

Multiple `Requires()` calls act as logical **AND** — all must be met for the scope to run:

```lua
Sequence("Advanced", function()
    Requires(Sequence("Setup"):ToPass())         -- Setup must pass
    Requires(Sequence("Calibration"):ToPass())   -- AND Calibration must pass
    Requires(Sequence():ToBeAfter())             -- AND runs after both
    -- ...
end)
```

If any single requirement is unmet, the scope is skipped.

---

## Common Patterns

### Linear Sequence Ordering

```lua
Sequence("Step 1", function()
    Requires(Sequence():ToBeFirst())
    -- ...
end)

Sequence("Step 2", function()
    Requires(Sequence("Step 1"):ToPass())
    -- ...
end)

Sequence("Step 3", function()
    Requires(Sequence("Step 2"):ToPass())
    -- ...
end)
```

### Test Dependencies Within a Sequence

```lua
Sequence("Communication", function()
    Test("Open Port", function()
        -- ...
    end)

    Test("Handshake", function()
        Requires(Test("Open Port"):ToPass())
        -- ...
    end)

    Test("Data Exchange", function()
        Requires(Test("Handshake"):ToPass())
        -- ...
    end)
end)
```

### Cross-Sequence Test Dependencies

```lua
Sequence("Calibration", function()
    Test("Measure Reference", function()
        local ref = measureReference()
        Expect(ref, "Reference"):ToBeInRange(4.9, 5.1):ExportAs("ref_value")
    end)
end)

Sequence("Verification", function()
    Requires(Sequence("Calibration"):ToPass())

    Test("Compare to Reference", function()
        local ref = Sequence("Calibration"):Test("Measure Reference"):Value("ref_value")
        local measured = readVoltage()
        Expect(measured, "Matches Reference"):ToBeNear(ref, 0.1)
    end)
end)
```

### Always-Run Teardown

```lua
Sequence("Teardown", function()
    Requires(Sequence():ToBeLast())
    -- No :ToPass() requirement — runs regardless of previous failures

    Test("Power Off", function()
        disablePower()
    end)
end)
```

---

## Summary Table

| Method | Type | Effect |
|---|---|---|
| `:ToBeFirst()` | Order | Place scope first |
| `:ToBeLast()` | Order | Place scope last |
| `:ToBeBefore()` | Order | Referenced runs before current |
| `:ToBeAfter()` | Order | Referenced runs after current |
| `:ToBeRightBefore()` | Order | Adjacent before |
| `:ToBeRightAfter()` | Order | Adjacent after |
| `:ToPass()` | Runtime + Order | Run only if referenced passed |
| `:ToFail()` | Runtime + Order | Run only if referenced failed |
| `:ToBeComplete()` | Runtime + Order | Run only if referenced was not skipped |
| `:HasPassed()` | Query | Returns `true` if referenced passed |
| `:HasFailed()` | Query | Returns `true` if referenced failed |

---

## See Also

- [Sequences & Tests](sequences-and-tests.md) — `Requires()`, `Sequence()`, `Test()`
- [Test Lifecycle](../architecture/test-lifecycle.md) — how the sort algorithm uses order requirements
- [Expectations](expectations.md) — `:ExportAs()` for sharing values between scopes
