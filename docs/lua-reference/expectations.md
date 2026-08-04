# Expectations

`Expect(value, name)` creates an expectation — an assertion that records whether a measured
value meets a criterion. Expectations are the core of test verification in Frasy. They produce
pass/fail results that appear in test reports.

---

## `Expect(value, name, opt?)`

Creates an expectation object. Chain a **matcher** method to perform the assertion.

```lua
Expect(voltage, "Supply Voltage"):ToBeInRange(4.9, 5.1)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `value` | any | The measured value to assert against |
| `name` | `string` | Human-readable label (appears in reports) |
| `opt` | `table?` | Optional: `{ note = "...", extra = {...} }` |

**Returns:** An `Expectation` object with matcher and modifier methods.

**Options:**

| Field | Type | Description |
|---|---|---|
| `note` | `string?` | Additional note attached to the result (defaults to `name`) |
| `extra` | `table?` | Extra data stored alongside the result for debugging |
| `onErrorExtra` | `table?` | Extra data to be added only if test fails |
| `mandatory` | `boolean?` | Set Mandatory modifier (default to `false`) |
| `inverted` | `boolean?` | Set Inverted modifier (default to `false`) |

---

## Modifiers

Modifiers change how the expectation behaves. They are chainable and should appear **before**
the matcher.

### `Inverted`

Inverts the expectation. The assertion passes when the matcher would normally fail, and
vice versa.

```lua
Expect(status, "Status", { inverted = true }):ToBeEqual(0)     -- passes if status ≠ 0
```

### `Mandatory`

If this expectation fails, the **entire test stops immediately**. Remaining expectations in the
test are not evaluated.

```lua
Expect(connected, "Device Connected", { mandatory = true }):ToBeTrue()
-- If connected is false, test stops here with "Unmet Expectation"
```

Without `Mandatory`, a failing expectation marks the test as failed but allows subsequent
expectations to still run.

---

## Matchers

Each matcher asserts the value against a specific criterion.

They all returns an `ExpectationResult`

### `:ToBeTrue()`

Asserts that the value is boolean `true`.

```lua
Expect(flag, "Enable Flag"):ToBeTrue()
```

| Report Field | Value |
|---|---|
| `method` | `"ToBeTrue"` |
| `expected` | `true` |

---

### `:ToBeFalse()`

Asserts that the value is boolean `false`.

```lua
Expect(errorFlag, "Error Flag"):ToBeFalse()
```

| Report Field | Value |
|---|---|
| `method` | `"ToBeFalse"` |
| `expected` | `false` |

---

### `:ToBeEqual(expected)`

Asserts that the value equals `expected` (same type and value).

```lua
Expect(response, "ACK Byte"):ToBeEqual(0x06)
Expect(name, "Device Name"):ToBeEqual("SensorV2")
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `expected` | any | The exact value to compare against |

| Report Field | Value |
|---|---|
| `method` | `"ToBeEqual"` |
| `expected` | the expected value |

---

### `:ToBeNear(expected, deviation)`

Asserts that a numeric value is within ±`deviation` of `expected`.

```lua
Expect(voltage, "VCC"):ToBeNear(3.3, 0.3)  -- passes if 3.0 ≤ voltage ≤ 3.6
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `expected` | `number` | The center value |
| `deviation` | `number` | Maximum absolute deviation (always treated as positive) |

| Report Field | Value |
|---|---|
| `method` | `"ToBeNear"` |
| `expected` | center value |
| `deviation` | absolute deviation |
| `min` | `expected - deviation` |
| `max` | `expected + deviation` |

---

### `:ToBeInRange(min, max)`

Asserts that a numeric value is between `min` and `max` (inclusive).

```lua
Expect(temperature, "Board Temp"):ToBeInRange(20, 50)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `min` | `number` | Minimum acceptable value (inclusive) |
| `max` | `number` | Maximum acceptable value (inclusive) |

| Report Field | Value |
|---|---|
| `method` | `"ToBeInRange"` |
| `min` | minimum |
| `max` | maximum |

---

### `:ToBeInPercentage(expected, percentage)`

Asserts that a numeric value is within ±`percentage`% of `expected`.

```lua
Expect(voltage, "VCC"):ToBeInPercentage(3.3, 5.0)  -- passes if within ±5% of 3.3
-- i.e., 3.135 ≤ voltage ≤ 3.465
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `expected` | `number` | The nominal value |
| `percentage` | `number` | Allowed deviation as a percentage (e.g., `5.0` for ±5%) |

| Report Field | Value |
|---|---|
| `method` | `"ToBeInPercentage"` |
| `expected` | nominal value |
| `percentage` | the percentage |
| `deviation` | computed absolute deviation (`expected * percentage / 100`) |
| `min` | `expected - deviation` |
| `max` | `expected + deviation` |

---

### `:ToBeGreater(min)`

Asserts that a numeric value is **strictly greater than** `min`.

```lua
Expect(current, "Active Current"):ToBeGreater(0.01)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `min` | `number` | The exclusive lower bound |

| Report Field | Value |
|---|---|
| `method` | `"ToBeGreater"` |
| `min` | the lower bound |

---

### `:ToBeGreaterOrEqual(min)`

Asserts that a numeric value is **greater than or equal to** `min`.

```lua
Expect(count, "Sample Count"):ToBeGreaterOrEqual(100)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `min` | `number` | The inclusive lower bound |

| Report Field | Value |
|---|---|
| `method` | `"ToBeGreaterOrEqual"` |
| `min` | the lower bound |

---

### `:ToBeLesser(max)`

Asserts that a numeric value is **strictly less than** `max`.

```lua
Expect(leakage, "Leakage Current"):ToBeLesser(0.001)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `max` | `number` | The exclusive upper bound |

| Report Field | Value |
|---|---|
| `method` | `"ToBeLesser"` |
| `max` | the upper bound |

---

### `:ToBeLesserOrEqual(max)`

Asserts that a numeric value is **less than or equal to** `max`.

```lua
Expect(ripple, "Ripple"):ToBeLesserOrEqual(50)
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `max` | `number` | The inclusive upper bound |

| Report Field | Value |
|---|---|
| `method` | `"ToBeLesserOrEqual"` |
| `max` | the upper bound |

---

### `:ToBeType(expected)`

Asserts that the Lua type of the value matches `expected`.

```lua
Expect(result, "Result Type"):ToBeType("table")
Expect(version, "Version Type"):ToBeType("string")
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `expected` | `string` | Expected Lua type (`"number"`, `"string"`, `"boolean"`, `"table"`, etc.) |

| Report Field | Value |
|---|---|
| `method` | `"ToBeType"` |
| `expected` | the expected type string |
| `type` | the actual type of the value |

---

### `:ToMatch(pattern)`

Asserts that a string value matches a Lua pattern.

```lua
Expect(serial, "Serial Format"):ToMatch("^%d%d%-%d%d%d%d%d%d$")
Expect(version, "Version Format"):ToMatch("%d+%.%d+%.%d+")
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `pattern` | `string` | A [Lua pattern](https://www.lua.org/pil/20.2.html) |

| Report Field | Value |
|---|---|
| `method` | `"ToMatch"` |
| `pattern` | the pattern string |

---

## Post-Assertion Methods

These methods can be called on the `ExpectationResult`

### `:ExportAs(name)`

Stores the measured value in the orchestrator's value store, making it available to other tests
via `Orchestrator.GetValue()`.

```lua
Expect(voltage, "Reference"):ToBeInRange(4.9, 5.1):ExportAs("ref_voltage")
```

### `:Show()`

Sends the expectation result to the C++ side for live display in the UI. Useful for real-time
monitoring during long test sequences.

```lua
Expect(voltage, "Live Voltage"):ToBeInRange(4.9, 5.1):Show()
```

---

## Stage Behavior

| Stage | Behavior |
|---|---|
| **Generation** | All expectations return `pass = true` immediately. No assertions are made. |
| **Validation** | All expectations return `pass = true` immediately. No assertions are made. |
| **Execution** | Expectations perform real assertions and record results. |

This allows test bodies to run during generation (to discover sequences, tests, and
requirements) without triggering false failures.

---

## How Results Appear in Reports

Each expectation produces a result entry in the test report:

```json
{
    "name": "Supply Voltage",
    "method": "ToBeInPercentage",
    "value": 3.28,
    "expected": 3.3,
    "percentage": 5.0,
    "deviation": 0.165,
    "min": 3.135,
    "max": 3.465,
    "pass": true,
    "inverted": false
}
```

---

## Complete Example

```lua
Test("Full Validation", function()
    local daq = Context.map.ibs.daq --[[@as DAQ]]

    -- Mandatory check — stops the test if it fails
    local connected = checkConnection()
    Expect(connected, "Board Connected"):Mandatory():ToBeTrue()

    -- Voltage within percentage
    local vcc = daq:MeasureVoltage(Context.values.route.vcc)
    Expect(vcc.average, "VCC"):ToBeInPercentage(3.3, 5.0)

    -- Current below threshold
    local current = daq:MeasureCurrent(Context.values.route.supply)
    Expect(current.average, "Idle Current"):ToBeLesser(0.100)

    -- Inverted check — ensure no error condition
    local errorBit = readStatus()
    Expect(errorBit, "Error Bit"):Not():ToBeTrue()

    -- String pattern matching
    local fw = readFirmwareVersion()
    Expect(fw, "FW Version"):ToMatch("%d+%.%d+%.%d+")

    -- With extra debug info on failure
    local response = sendCommand(0x01)
    Expect(response, "ACK"):ToBeEqual(0x06):OnErrorExtra({
        raw = response,
        expected = 0x06,
    })
end)
```

---

## See Also

- [Sequences & Tests](sequences-and-tests.md) — where expectations are used
- [Requirements](requirements.md) — conditional execution based on pass/fail
- [Test Lifecycle](../architecture/test-lifecycle.md) — how expectations are collected and reported
