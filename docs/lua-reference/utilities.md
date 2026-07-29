# Utilities

Frasy provides a set of global helper functions and importable modules for common operations:
sleeping, string conversion, table traversal, retries, timeouts, and bitwise manipulation.

---

## Global Functions

These are available everywhere without `require()`.

### `SleepFor(ms)`

Pauses the current UUT's execution for the specified duration.

```lua
SleepFor(200)  -- wait 200 milliseconds
```

| Parameter | Type | Description |
|---|---|---|
| `ms` | `integer` | Duration in milliseconds |

**Notes:**

- During generation/validation, `SleepFor` is a no-op (returns immediately).
- Only the calling UUT's thread sleeps — other UUTs continue running.

---

### `ToString(t, max_depth?)`

Converts any Lua value to a string. Unlike `tostring()`, it recursively prints table contents.
Respects `__tostring` metamethods.

```lua
local s = ToString({ a = 1, b = { c = 2 } })
Log.D(s)
-- a: 1
-- b:
--   c: 2
```

| Parameter | Type | Description |
|---|---|---|
| `t` | any | Value to convert |
| `max_depth` | `number?` | Maximum table depth (default: unlimited) |

**Returns:** `string`

---

### `Print(t, max_depth?)`

Prints any Lua value to stdout. Like `ToString()` but outputs directly via `print()`.

```lua
Print(myTable, 2)  -- print up to 2 levels deep
```

| Parameter | Type | Description |
|---|---|---|
| `t` | any | Value to print |
| `max_depth` | `number?` | Maximum table depth |

---

### `Equals(t1, t2)`

Deep equality comparison. Unlike `==`, it compares table contents recursively rather than
object references.

```lua
local a = { x = 1, y = { z = 2 } }
local b = { x = 1, y = { z = 2 } }
Equals(a, b)  -- true (same content)
a == b        -- false (different references)
```

| Parameter | Type | Description |
|---|---|---|
| `t1` | any | First value |
| `t2` | any | Second value |

**Returns:** `boolean`

---

### `Traverse(t, ...)`

Safely accesses nested table values via a chain of keys. Returns `nil` if any key in the chain
is missing (instead of erroring).

```lua
local config = { display = { brightness = 80 } }

Traverse(config, "display", "brightness")  -- 80
Traverse(config, "display", "contrast")    -- nil (missing key)
Traverse(config, "audio", "volume")        -- nil (broken chain)
```

| Parameter | Type | Description |
|---|---|---|
| `t` | `table` | Table to traverse |
| `...` | `string\|number` | Chain of keys |

**Returns:** The value at the end of the chain, or `nil` if any key is missing.

---

### `ToInt(value)`

Rounds a number to the nearest integer.

```lua
ToInt(3.7)   -- 4
ToInt(3.2)   -- 3
ToInt(-1.6)  -- -2
```

| Parameter | Type | Description |
|---|---|---|
| `value` | `number` | Number to round |

**Returns:** `integer`

---

### `Hash(str)`

Hashes a string using a fast hash function. Returns an integer.

```lua
local h = Hash("my_unique_key")
```

| Parameter | Type | Description |
|---|---|---|
| `str` | `string` | String to hash |

**Returns:** `integer`

**Notes:** Used internally by `Once()` to identify unique call sites. Can also be used to
generate deterministic IDs from strings.

---

### `DirList(path)`

Lists all `.lua` files recursively in a directory. Returns their paths without the `.lua`
extension.

```lua
local files = DirList("lua/user/my_product/tests")
for _, f in ipairs(files) do
    Log.D("Found: " .. f)
end
```

| Parameter | Type | Description |
|---|---|---|
| `path` | `string` | Directory path to scan |

**Returns:** `string[]` — array of file paths (without `.lua` extension).

---

### `SaveAsJson(table, filepath)`

Serializes a Lua table to a JSON file on disk.

```lua
SaveAsJson({ voltage = 3.3, pass = true }, "logs/debug_data.json")
```

| Parameter | Type | Description |
|---|---|---|
| `table` | `table` | Data to serialize |
| `filepath` | `string` | Output file path |

---

### `CombineAndBitcast(data)`

Combines 4 bytes (little-endian) and interprets them as a 32-bit float. Useful for decoding
raw sensor data.

```lua
local raw = { 0x00, 0x00, 0x48, 0x42 }  -- 50.0 as IEEE 754
local value = CombineAndBitcast(raw)      -- 50.0
```

| Parameter | Type | Description |
|---|---|---|
| `data` | `integer[4]` | Array of 4 bytes |

**Returns:** `number` — the decoded float value.

---

### `LineSplit(content)`

Splits a string into an array of lines.

```lua
local lines = LineSplit("line1\nline2\nline3")
-- lines = {"line1", "line2", "line3"}
```

| Parameter | Type | Description |
|---|---|---|
| `content` | `string` | Text to split |

**Returns:** `string[]`

---

## Importable Modules

These must be loaded with `require()`.

### `TryFunction` — Retry Logic

Calls a function repeatedly until it returns `true` or a maximum number of attempts is
reached.

```lua
local TryFunction = require("lua/core/utils/try_function")

local success, result = TryFunction(function(attempt)
    Log.D("Attempt " .. attempt)
    local response = sendCommand(0x01)
    return response == 0x06, response
end, { maxTryCount = 5, delay = 100, raiseError = true })
```

**Signature:** `TryFunction(fn, opt?) → boolean, any?`

| Parameter | Type | Description |
|---|---|---|
| `fn` | `function(attempt) → boolean, any?` | Function to try. Receives attempt number (1-indexed). Return `true` to stop retrying. Optional second return value is passed through. |
| `opt` | `table?` | Options (see below) |

**Options:**

| Field | Type | Default | Description |
|---|---|---|---|
| `maxTryCount` | `integer` | `3` | Maximum number of attempts |
| `delay` | `integer` | `10` | Delay in ms between attempts |
| `raiseError` | `boolean` | `false` | If `true`, throws an error when max attempts is reached |

**Returns:** `success` (boolean), and optionally the second return value from `fn`.

---

### `TimeoutFunction` — Wait with Timeout

Polls a condition function repeatedly until it returns `false` (condition met) or the timeout
is reached.

```lua
local TimeoutFunction = require("lua/core/utils/timeout_function")

-- Wait up to 5 seconds for the device to become ready
TimeoutFunction(function()
    return not isDeviceReady()  -- return true to keep waiting, false to stop
end, 5000, 50)
```

**Signature:** `TimeoutFunction(routine, duration_ms, sleep_ms?)`

| Parameter | Type | Description |
|---|---|---|
| `routine` | `function() → boolean` | Return `true` to keep waiting, `false` when condition is met |
| `duration_ms` | `integer` | Maximum wait time in milliseconds |
| `sleep_ms` | `integer?` | Sleep between polls (default: 10ms) |

**Behavior:**

- Throws `"Timeout"` error if `duration_ms` is exceeded during execution.
- During generation/validation, returns immediately without error.

---

### `Bitwise` — Bit Manipulation

Helpers for injecting and extracting individual bits in an integer (useful for GPIO registers).

```lua
local Bitwise = require("lua/core/utils/bitwise")

-- Set bit 3 to 1 in a cache value
local cache = 0x00
cache = Bitwise.Inject(3, 1, cache)  -- cache = 0x08

-- Read bit 3
local bit = Bitwise.Extract(3, cache)  -- bit = 1
```

#### `Bitwise.Inject(index, value, cache)`

Sets a single bit in an integer.

| Parameter | Type | Description |
|---|---|---|
| `index` | `integer` | Bit position (0-indexed) |
| `value` | `integer` | Bit value (0 or 1) |
| `cache` | `integer` | The integer to modify |

**Returns:** The modified integer.

#### `Bitwise.Extract(index, value)`

Reads a single bit from an integer.

| Parameter | Type | Description |
|---|---|---|
| `index` | `integer` | Bit position (0-indexed) |
| `value` | `integer` | The integer to read from |

**Returns:** `0` or `1`.

---

### `StringizeValues` — Pack Bytes to String

Packs a list of byte values into a binary string using `string.pack`.

```lua
local StringizeValues = require("lua/core/utils/stringize_values")

local packet = StringizeValues(0x01, 0x02, 0x03, 0x04)
-- packet = "\x01\x02\x03\x04"
```

**Signature:** `StringizeValues(...) → string`

| Parameter | Type | Description |
|---|---|---|
| `...` | `integer...` | Byte values (0–255) |

**Returns:** A binary string with each byte packed sequentially.

---

## See Also

- [Validation & Assertions](validation.md) — `CheckField`, `Is`, `Maybe`
- [Logging](logging.md) — `Log.D/I/W/E` for debug output
- [Context](context.md) — `Context.info.stage` for stage-aware logic
