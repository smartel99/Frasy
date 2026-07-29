# Exposing C++ to Lua

Frasy uses [sol2](https://github.com/ThePhD/sol2) to bridge C++ and Lua. The orchestrator
provides three extension callbacks that let you inject custom functions, board definitions, and
runtime values into the Lua environment — all without modifying the framework itself.

---

## Extension Callbacks Overview

| Callback | Signature | Available in Lua as |
|---|---|---|
| `setLoadUserFunctions` | `void(sol::state_view)` | Global functions (e.g., `MyFunc()`) |
| `setLoadUserBoards` | `sol::table(sol::state_view)` | Entries in the `Ibs` table |
| `setLoadUserValues` | `sol::table(sol::state_view)` | `Context.values.gui` |

All three are called during the initialization phase, before any test files are executed. They
run once per orchestrator load (when a product is selected or reloaded).

---

## Registering the Callbacks

Register your callbacks in `makeOrchestrator()` (or wherever you set up the orchestrator after
loading a product):

```cpp
void MyMainApplicationLayer::makeOrchestrator(const std::string& name,
                                              const std::string& envPath,
                                              const std::string& testPath)
{
    if (m_orchestrator.loadUserFiles(envPath, testPath)) {
        // ... CANopen setup ...

        m_orchestrator.setLoadUserFunctions([&](sol::state_view lua) {
            loadLuaFunctions(lua);
        });

        m_orchestrator.setLoadUserValues([&](sol::state_view lua) -> sol::table {
            return loadGuiValues(lua);
        });

        m_orchestrator.setLoadUserBoards([&](sol::state_view lua) -> sol::table {
            return loadCustomBoards(lua);
        });
    }
}
```

---

## Custom Functions (`setLoadUserFunctions`)

This is the most common extension point. Use it to expose C++ functions that test scripts can
call directly.

### Simple Functions

```cpp
void MyMainApplicationLayer::loadLuaFunctions(sol::state_view lua)
{
    // A simple function with no arguments
    lua["GetTimestamp"] = []() -> double {
        using namespace std::chrono;
        return duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
        ).count() / 1000.0;
    };

    // A function with parameters and a return value
    lua["ComputeChecksum"] = [](const std::string& data) -> uint32_t {
        return crc32(data.data(), data.size());
    };

    // A function that interacts with hardware
    lua["SetExternalRelay"] = [this](int channel, bool state) {
        m_relayController.set(channel, state);
    };
}
```

In Lua:

```lua
Test("Timing Check", function()
    local start = GetTimestamp()
    -- ... do work ...
    local elapsed = GetTimestamp() - start
    Expect(elapsed, "Duration"):ToBeLesser(2.0)
end)
```

### Functions in Tables (Namespacing)

Group related functions under a table to avoid polluting the global namespace:

```cpp
void MyMainApplicationLayer::loadLuaFunctions(sol::state_view lua)
{
    auto fixture = lua.create_table();
    fixture["EnablePower"]  = [this]() { m_powerSupply.enable(); };
    fixture["DisablePower"] = [this]() { m_powerSupply.disable(); };
    fixture["SetVoltage"]   = [this](double volts) { m_powerSupply.setVoltage(volts); };
    fixture["ReadCurrent"]  = [this]() -> double { return m_powerSupply.readCurrent(); };
    lua["Fixture"] = fixture;
}
```

In Lua:

```lua
Test("Power Supply", function()
    Fixture.SetVoltage(12.0)
    Fixture.EnablePower()
    SleepFor(100)
    local current = Fixture.ReadCurrent()
    Expect(current, "Idle Current"):ToBeLesser(0.5)
    Fixture.DisablePower()
end)
```

### Overloaded Functions

sol2 supports overloading with `sol::overload`:

```cpp
lua["MeasureVoltage"] = sol::overload(
    [this](int channel) -> double {
        return m_dmm.measure(channel);
    },
    [this](int channel, int samples) -> double {
        return m_dmm.measureAverage(channel, samples);
    }
);
```

### Error Handling

Throw `sol::error` to raise a Lua error that the orchestrator will catch and report:

```cpp
lua["ReadSensor"] = [this](int id) -> double {
    if (id < 0 || id >= m_sensorCount) {
        throw sol::error(std::format("Invalid sensor ID: {} (valid: 0-{})", id, m_sensorCount - 1));
    }
    auto value = m_sensor.read(id);
    if (!value.has_value()) {
        throw sol::error(std::format("Sensor {} read timeout", id));
    }
    return *value;
};
```

In Lua, this surfaces as a test failure with the error message in the report.

---

## GUI Values (`setLoadUserValues`)

Use this callback to pass runtime values from the UI (operator inputs, dropdown selections,
fixture config) into Lua. The returned table is available at `Context.values.gui`.

```cpp
sol::table MyMainApplicationLayer::loadGuiValues(sol::state_view lua)
{
    auto t = lua.create_table();
    t["operator"]    = std::string(m_operatorName.data());
    t["temperature"] = m_ambientTemperature;
    t["fixture_id"]  = m_fixtureId;
    return t;
}
```

In Lua:

```lua
Test("Report Context", function()
    Log.I("Operator: " .. Context.values.gui.operator)
    Log.I("Ambient temp: " .. tostring(Context.values.gui.temperature))
end)
```

!!! tip
    `Context.values.gui` is ideal for values that change between runs (operator name, lot
    number, station ID) but don't belong in the product's `environment.lua`.

---

## Custom Boards (`setLoadUserBoards`)

If you have custom hardware that isn't managed through the standard `Environment.Ib.Add()`
path (e.g., non-CANopen instruments, GPIO expanders over I²C), you can inject them into the
`Ibs` table directly:

```cpp
sol::table MyMainApplicationLayer::loadCustomBoards(sol::state_view lua)
{
    auto boards = lua.create_table();

    auto dmm = lua.create_table();
    dmm["measure"] = [this](int channel) -> double {
        return m_dmm.measure(channel);
    };
    dmm["setRange"] = [this](const std::string& range) {
        m_dmm.setRange(range);
    };
    boards["dmm"] = dmm;

    return boards;
}
```

The returned table's entries are merged into the global `Ibs` table. In Lua:

```lua
Test("DMM Reading", function()
    Ibs.dmm.setRange("auto")
    local v = Ibs.dmm.measure(1)
    Expect(v, "Voltage"):ToBeInRange(4.9, 5.1)
end)
```

!!! note
    For CANopen-based boards, prefer defining them in Lua with `Environment.Ib.Add()` and a
    proper EDS file. Use `setLoadUserBoards` only for instruments that don't fit the CANopen
    model.

---

## Stage Awareness

Your custom functions are available during **all** stages (generation, validation, execution).
If a function performs hardware I/O that should only happen during execution, guard it:

```cpp
lua["ReadADC"] = [this](sol::this_state state, int channel) -> double {
    sol::state_view luaView(state.lua_state());
    auto stage = luaView["Context"]["info"]["stage"].get<std::string>();
    if (stage != "execution") {
        return 0.0;  // Return a dummy value during generation/validation
    }
    return m_adc.read(channel);
};
```

Alternatively, check the stage in Lua:

```lua
Test("ADC Check", function()
    if Context.info.stage ~= Stage.execution then return end
    local v = ReadADC(3)
    Expect(v, "ADC Channel 3"):ToBeInRange(1.0, 2.0)
end)
```

!!! warning
    The framework's built-in hardware functions (SDO upload/download) already guard themselves
    with stage checks. For consistency, your custom hardware functions should do the same.

---

## Supported Types

sol2 automatically converts between C++ and Lua types:

| C++ Type | Lua Type |
|---|---|
| `int`, `double`, `float`, etc. | `number` |
| `bool` | `boolean` |
| `std::string`, `std::string_view` | `string` |
| `sol::table` | `table` |
| `sol::object` | any |
| `std::vector<T>` | table (array) |
| `std::map<K,V>` | table (dictionary) |
| `std::optional<T>` | value or `nil` |
| `sol::nil_t` | `nil` |

### Returning Tables

```cpp
lua["GetMeasurement"] = [this](sol::this_state state, int channel) -> sol::table {
    sol::state_view lua(state.lua_state());
    auto t = lua.create_table();
    auto result = m_daq.measure(channel);
    t["average"] = result.average;
    t["min"]     = result.min;
    t["max"]     = result.max;
    t["samples"] = result.sampleCount;
    return t;
};
```

### Receiving Tables

```cpp
lua["ConfigureTest"] = [](sol::table config) {
    auto timeout = config.get_or<int>("timeout", 1000);
    auto retries = config.get_or<int>("retries", 3);
    auto channel = config["channel"].get<int>();
    // ...
};
```

---

## Thread Safety

During execution, each UUT runs in its own thread. If your custom functions access shared
resources, you must synchronize access:

```cpp
void MyMainApplicationLayer::loadLuaFunctions(sol::state_view lua)
{
    lua["SharedWrite"] = [this](int address, int value) {
        std::lock_guard lock(m_hardwareMutex);
        m_device.write(address, value);
    };
}
```

Alternatively, if the resource is per-UUT, use the UUT index to select the right instance:

```cpp
lua["ReadChannel"] = [this](sol::this_state state) -> double {
    sol::state_view lua(state.lua_state());
    int uut = lua["Context"]["info"]["uut"].get<int>();
    return m_channels[uut].read();
};
```

!!! tip
    Frasy provides `Exclusive(id, fn)` in Lua for serializing access across UUTs. If your
    function is always called inside an `Exclusive` block, you may not need C++-level locking.

---

## Complete Example

```cpp
void MyMainApplicationLayer::loadLuaFunctions(sol::state_view lua)
{
    // Simple utility
    lua["FormatSerial"] = [](const std::string& prefix, int number) -> std::string {
        return std::format("{}-{:06d}", prefix, number);
    };

    // Hardware interaction (stage-aware)
    lua["PowerSupply"] = lua.create_table_with(
        "Enable", [this](sol::this_state s) {
            if (getStage(s) != "execution") return;
            m_psu.enable();
        },
        "Disable", [this](sol::this_state s) {
            if (getStage(s) != "execution") return;
            m_psu.disable();
        },
        "SetVoltage", [this](sol::this_state s, double v) {
            if (getStage(s) != "execution") return;
            m_psu.setVoltage(v);
        },
        "MeasureCurrent", [this](sol::this_state s) -> double {
            if (getStage(s) != "execution") return 0.0;
            return m_psu.measureCurrent();
        }
    );
}

// Helper to read stage from Lua state
std::string MyMainApplicationLayer::getStage(sol::this_state state) {
    sol::state_view lua(state.lua_state());
    return lua["Context"]["info"]["stage"].get<std::string>();
}
```

Usage in Lua:

```lua
Sequence("Power Tests", function()
    Test("Current Draw", function()
        PowerSupply.SetVoltage(12.0)
        PowerSupply.Enable()
        SleepFor(200)
        local i = PowerSupply.MeasureCurrent()
        Expect(i, "Supply Current"):ToBeLesser(0.5)
        PowerSupply.Disable()
    end)
end)
```

---

## Tips

- **Name functions clearly.** Test engineers reading Lua scripts should immediately understand
  what a function does without looking at the C++ source.
- **Return structured data.** Prefer returning a table with named fields (e.g.,
  `{average=..., min=..., max=...}`) over multiple return values.
- **Guard hardware I/O.** Always check the stage or return dummy values during
  generation/validation. Accidentally triggering hardware during generation can damage
  equipment or produce confusing errors.
- **Keep it stateless when possible.** Functions that depend on external state (open
  connections, prior configuration) are harder to debug. Document prerequisites in comments.
- **Use `sol::this_state`** when you need access to the Lua state (for creating tables,
  reading `Context`, or accessing other Lua globals) from within your C++ lambda.
