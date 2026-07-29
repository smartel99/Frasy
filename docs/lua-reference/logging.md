# Logging

The `Log` global provides functions to write messages to Frasy's Log Window panel. Messages are
tagged with the UUT number automatically (e.g., `UUT1`, `UUT2`) so you can identify which UUT
produced each log entry in multi-UUT environments.

---

## Log Levels

| Function | Level | Color | Use For |
|---|---|---|---|
| `Log.T(message)` | Trace | Gray | Extremely verbose diagnostics (usually filtered out) |
| `Log.D(message)` | Debug | Cyan | Development-time debugging info |
| `Log.I(message)` | Info | Green | Normal operational messages |
| `Log.W(message)` | Warning | Yellow | Unexpected but recoverable situations |
| `Log.E(message)` | Error | Red | Failures that affect test results |
| `Log.C(message)` | Critical | Red/Bold | Unrecoverable errors |

All functions take a single string argument. They do not support format strings directly —
use `string.format()` or concatenation.

---

## Usage

```lua
Log.I("Starting power-on sequence")
Log.D("Voltage reading: " .. tostring(voltage))
Log.W("Current is near the upper limit")
Log.E("Communication timeout after 3 retries")
```

### With String Formatting

```lua
Log.I(string.format("Channel %d: %.3f V", channel, voltage))
Log.D(string.format("Attempt %d/%d", retry, maxRetries))
Log.W(string.format("Temperature %.1f°C exceeds threshold", temp))
```

---

## Where Messages Appear

Log messages appear in the **Log Window** panel (++f2++). Each entry shows:

- **Timestamp** — when the message was produced
- **Source** — the UUT tag (e.g., `UUT1`)
- **Level** — the severity level
- **Message** — your text
- **Source location** — file and line number (if enabled in config)

Messages are also written to the application's log file on disk.

---

## Filtering

The Log Window panel supports filtering by:

- **Level** — toggle visibility per level (trace, debug, info, warn, error, critical)
- **Text** — free-text filter in the search bar
- **Source** — per-logger level overrides in `config.json`

To reduce noise from a specific subsystem at the config level:

```json
"LogWindow": {
    "Loggers": {
        "SlCan": 2
    }
}
```

Logger levels are integers matching spdlog levels: 0 = trace, 1 = debug, 2 = info, 3 = warn,
4 = error, 5 = critical.

---

## Behavior During Stages

Logging works in **all stages** (generation, validation, execution). This can be useful for
debugging environment loading or requirement resolution.

```lua
Log.D("Environment loaded, UUT count: " .. #Context.map.uuts)
```

---

## Best Practices

- **Use `Log.I` for test progress.** Log when a sequence or significant step starts/completes.
  The orchestrator already logs test pass/fail — don't duplicate that.
- **Use `Log.D` for measurements.** Log raw values before assertion so failures can be
  diagnosed from the log without re-running.
- **Use `Log.W` for soft limits.** If a value passes but is close to the boundary, warn so
  operators notice drift.
- **Use `Log.E` sparingly.** The framework logs errors for failed expectations automatically.
  Only use `Log.E` for issues that aren't captured by expectations (hardware communication
  problems, unexpected states).
- **Avoid logging in tight loops.** Logging is relatively expensive. Don't log inside
  measurement averaging loops or polling loops — log the summary instead.
- **Use `string.format` for numeric values.** Truncate floats to reasonable precision:
  `string.format("%.3f", v)` rather than `tostring(v)` which may produce excessive decimals.

---

## Example

```lua
Sequence("Communication", function()
    Test("RS485 Echo", function()
        Log.I("Testing RS485 echo at 115200 baud")

        local sent = 0xA5
        local received = rs485Send(sent)

        Log.D(string.format("Sent: 0x%02X, Received: 0x%02X", sent, received or 0))

        if received == nil then
            Log.E("No response from RS485 device")
        end

        Expect(received, "Echo Response"):ToBeEqual(sent)
    end)
end)
```

---

## See Also

- [Configuration](../developer-guide/configuration.md) — `LogWindow` config keys and per-logger levels
- [Panels](../panels/index.md) — Log Window panel reference
