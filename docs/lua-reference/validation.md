# Validation & Assertions

Frasy provides three utilities for validating function arguments and data at runtime:
`CheckField`, `Is`, and `Maybe`. These are available as globals in all Lua scripts (loaded by
the framework at startup via `checkers.lua`).

They are used extensively inside board drivers and helper functions to catch programming errors
early with clear error messages — before they reach hardware or corrupt test results.

---

## `CheckField(value, predicate, ...)`

Asserts that a value satisfies a predicate. Throws an error with the variable name and value
if validation fails. Returns the value on success (for use in expressions).

```lua
CheckField(voltage, Is.Float)
CheckField(channel, Is.IntegerIn, 1, 4)
CheckField(mode, Is.Boolean)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `value` | any | The value to validate |
| `predicate` | `function(v, ...) → boolean` | A validation function (typically from `Is`) |
| `...` | any | Additional arguments passed to the predicate |

**Returns:** `value` (pass-through on success).

**On failure:** Throws an error with the variable name (auto-detected from the source line)
and the invalid value.

### Example Error

```lua
local temperature = "not a number"
CheckField(temperature, Is.Float)
-- Error: CheckField failed. temperature: not a number
```

### Using the Return Value

Since `CheckField` returns the value, you can use it inline:

```lua
function MyBoard:SetChannel(ch)
    self.channel = CheckField(ch, Is.IntegerIn, 1, 8)
end
```

---

## `Is`

A table of predicate functions for type and range checking. Each function takes a value as its
first argument and returns `true` or `false`.

### Type Predicates

| Predicate | Description |
|---|---|
| `Is.Nil(v)` | `v` is `nil` |
| `Is.Boolean(v)` | `v` is a boolean |
| `Is.Number(v)` | `v` is a number (integer or float) |
| `Is.Float(v)` | `v` is a number (any numeric value) |
| `Is.Integer(v)` | `v` is a whole number (no fractional part) |
| `Is.Unsigned(v)` | `v` is a non-negative integer (≥ 0) |
| `Is.String(v)` | `v` is a string |
| `Is.Table(v)` | `v` is a table |
| `Is.Function(v)` | `v` is a function |
| `Is.Array(v)` | `v` is a non-empty table with sequential integer keys |

### Sized Integer Predicates

| Predicate | Range |
|---|---|
| `Is.Integer8(v)` | -128 to 127 |
| `Is.Integer16(v)` | -32,768 to 32,767 |
| `Is.Integer32(v)` | -2,147,483,648 to 2,147,483,647 |
| `Is.Unsigned8(v)` | 0 to 255 |
| `Is.Unsigned16(v)` | 0 to 65,535 |
| `Is.Unsigned32(v)` | 0 to 4,294,967,295 |

### Range Predicates (Inclusive)

| Predicate | Description |
|---|---|
| `Is.IntegerIn(v, min, max)` | Integer where `min ≤ v ≤ max` |
| `Is.UnsignedIn(v, min, max)` | Unsigned integer where `min ≤ v ≤ max` |
| `Is.FloatIn(v, min, max)` | Number where `min ≤ v ≤ max` |

### Range Predicates (Exclusive)

| Predicate | Description |
|---|---|
| `Is.IntegerInEx(v, min, max)` | Integer where `min < v < max` |
| `Is.UnsignedInEx(v, min, max)` | Unsigned integer where `min < v < max` |
| `Is.FloatInEx(v, min, max)` | Number where `min < v < max` |

### Object Dictionary Predicates

These validate a value against limits defined in a CANopen object dictionary entry:

| Predicate | Description |
|---|---|
| `Is.IntegerInOd(v, od)` | Integer within `od.lowLimit` to `od.highLimit` |
| `Is.UnsignedInOd(v, od)` | Unsigned integer within OD limits |
| `Is.FloatInOd(v, od)` | Float within OD limits |
| `Is.ArrayInOd(v, od)` | Array with length ≤ `od.stringLengthMin` |

### Collection Predicates

| Predicate | Description |
|---|---|
| `Is.Array(v)` | Value is a non-empty table |
| `Is.ArrayInOd(v, od)` | Array with length within OD bounds |
| `Is.InArray(v, t)` | Value `v` exists in array `t` |

### Negation

| Predicate | Description |
|---|---|
| `Is.Not(v, f, ...)` | Inverts any predicate: `not f(v, ...)` |

```lua
CheckField(value, Is.Not, Is.Nil)          -- value must not be nil
CheckField(mode, Is.Not, Is.Integer8)      -- mode must not fit in 8 bits
```

---

## `Maybe(value, predicate, ...)`

Validates that a value is **either `nil` or satisfies the predicate**. Useful for optional
parameters.

```lua
Maybe(timeout, Is.Integer)       -- nil is OK, but if provided must be an integer
Maybe(name, Is.String)           -- nil is OK, but if provided must be a string
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `value` | any | The value to check (may be `nil`) |
| `predicate` | `function(v, ...) → boolean` | Validation function |
| `...` | any | Additional arguments for the predicate |

**Returns:** `true` if the value is `nil` or satisfies the predicate, `false` otherwise.

### Usage with CheckField

Combine `Maybe` with `CheckField` for optional parameter validation:

```lua
function MyBoard:Configure(channel, timeout)
    CheckField(channel, Is.IntegerIn, 1, 8)
    CheckField(timeout, Maybe, Is.Integer)  -- timeout is optional
end
```

!!! note
    `Maybe` returns a boolean — it doesn't throw. Wrap it in `CheckField` if you want an
    error on failure.

---

## Common Patterns

### Validating Function Parameters

```lua
function DAQ:MeasureVoltage(routingPoint, samples)
    CheckField(routingPoint, Is.Unsigned16)
    CheckField(samples, Maybe, Is.IntegerIn, 1, 1000)
    -- ...
end
```

### Validating Against an Enum

```lua
local Mode = { off = 0, low = 1, high = 2 }

function setMode(mode)
    CheckField(mode, Is.IntegerIn, Mode.off, Mode.high)
    -- ...
end
```

### Validating OD Bounds Before Download

```lua
function PIO:SupplyCurrentLimit(supply, limit)
    CheckField(supply, Is.UnsignedIn, PIO.SupplyEnum.p3v3, PIO.SupplyEnum.pVariable2)
    local od = self.ib.od[PIO.SupplyEnumToOdName(supply)]["Current Limit"]
    CheckField(limit, Is.IntegerInOd, od)
    self.ib:Download(od, limit)
end
```

### Optional Parameters with Defaults

```lua
function measure(channel, opt)
    CheckField(channel, Is.IntegerIn, 1, 4)
    if opt ~= nil then
        CheckField(opt, Is.Table)
        CheckField(opt.samples, Maybe, Is.IntegerIn, 1, 10000)
        CheckField(opt.timeout, Maybe, Is.Integer)
    end
    local samples = (opt and opt.samples) or 100
    local timeout = (opt and opt.timeout) or 1000
    -- ...
end
```

### Negation for Exclusion

```lua
-- Ensure value is not nil (required parameter)
CheckField(callback, Is.Not, Is.Nil)

-- Ensure value is not zero
CheckField(divisor, Is.Not, Is.IntegerIn, 0, 0)
```

---

## Best Practices

- **Validate at function boundaries.** Check parameters at the top of functions — before any
  hardware interaction or state mutation.
- **Use `Is.*` predicates, not raw type checks.** `CheckField(v, Is.Integer)` gives better
  error messages than `assert(type(v) == "number")` and also verifies it's a whole number.
- **Use `Maybe` for optional parameters.** It clearly documents intent and avoids nil-check
  boilerplate.
- **Validate OD bounds before download.** Sending an out-of-range value to hardware can cause
  undefined behavior. Use `Is.IntegerInOd` / `Is.FloatInOd` to catch this in software.
- **Keep CheckField close to the call site.** Validate where the data enters your function,
  not deep inside nested logic. This produces clearer error messages.

---

## See Also

- [Custom Instrumentation Boards](../developer-guide/custom-ibs.md) — using CheckField in board method implementations
- [Environment](environment.md) — how boards and their OD entries are set up
- [Utilities](utilities.md) — other helper functions available in scripts
