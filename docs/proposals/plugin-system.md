# Proposal: Native Plugin System

**Status:** Draft  
**Date:** 2026-08-08

---

## Problem

Some products require interfacing with hardware that has vendor-supplied C/C++ libraries (`.lib`, `.dll`, header files). A single executable cannot accommodate all possible hardware at compile time.

## Solution

A plugin system where native DLLs in a `plugins/` folder register Lua-callable functions at runtime. Plugins also have access to Frasy and Brigerad APIs and can render to the GUI.

---

## Plugin Directory Layout

```
<frasy-install>/
    frasy.exe
    plugins/
        visa.dll
        my_instrument.dll
    lua/
        user/
            my_product/
                environment.lua   ← declares Environment.Plugin.Load("visa")
                ...
```

---

## Plugin SDK (C++ header)

```cpp
// frasy_plugin.h — shipped with Frasy
#ifndef FRASY_PLUGIN_H
#define FRASY_PLUGIN_H

#include <lua.h>

#define FRASY_PLUGIN_ABI_VERSION 1

// Forward declarations for Frasy/Brigerad APIs available to plugins
namespace Frasy::CanOpen { class CanOpen; }
namespace Frasy::Lua { class Orchestrator; }

struct FrasyPluginContext {
    lua_State*              lua;        // Active Lua state
    Frasy::CanOpen::CanOpen* canOpen;   // CANopen bus access
    // Extensible: add more Frasy/Brigerad APIs here as needed
};

struct FrasyPluginInfo {
    int         abi_version;                // Must equal FRASY_PLUGIN_ABI_VERSION
    const char* name;                       // Plugin identifier (e.g., "visa")
    const char* version;                    // Plugin version string (e.g., "1.0.0")
    const char* description;                // Human-readable description
    int       (*init)(FrasyPluginContext* ctx);   // Called once on load
    void      (*shutdown)();                     // Called on application exit
    void      (*on_product_deselected)();        // Called on product switch (nullable)
    void      (*on_render)();                    // Called every frame for GUI rendering (nullable)
};

// Every plugin DLL must export this symbol:
// extern "C" __declspec(dllexport) FrasyPluginInfo* frasy_plugin_info();

#define FRASY_PLUGIN_EXPORT extern "C" __declspec(dllexport)

#endif // FRASY_PLUGIN_H
```

---

## Plugin Access to Frasy/Brigerad APIs

Plugins receive a `FrasyPluginContext` at initialization, giving them access to framework services:

```cpp
static FrasyPluginContext* s_ctx = nullptr;

static int plugin_init(FrasyPluginContext* ctx) {
    s_ctx = ctx;
    sol::state_view lua(ctx->lua);

    // Register Lua functions
    lua["MyInstrument"] = lua.create_table_with(
        "Open", []() { /* ... */ },
        "Measure", []() -> double { /* ... */ }
    );

    // Access CANopen if needed
    auto node = ctx->canOpen->getNode(30);

    return 0;
}
```

The plugin SDK ships with the necessary Frasy/Brigerad headers so plugins can use framework types and APIs directly. Plugins link against an import library (`frasy_plugin_sdk.lib`) or use the context pointer for runtime access.

---

## Plugin GUI Rendering

Plugins that need to render UI elements implement `on_render()`:

```cpp
static void plugin_on_render() {
    // Plugin can render its own ImGui panels
    if (ImGui::Begin("My Instrument Status")) {
        ImGui::Text("Connected: %s", s_connected ? "Yes" : "No");
        ImGui::Text("Last reading: %.3f V", s_lastReading);
        if (ImGui::Button("Reconnect")) {
            reconnect();
        }
    }
    ImGui::End();
}

static FrasyPluginInfo s_info {
    .abi_version            = FRASY_PLUGIN_ABI_VERSION,
    .name                   = "my_instrument",
    .version                = "1.0.0",
    .description            = "Custom instrument interface",
    .init                   = plugin_init,
    .shutdown               = plugin_shutdown,
    .on_product_deselected  = plugin_safe_state,
    .on_render              = plugin_on_render,  // called every frame
};
```

In headless mode, `on_render()` is never called.

---

## Example Plugin Implementation (Simple)

```cpp
// visa_plugin.cpp
#include <frasy_plugin.h>
#include <visa.h>
#include <sol/sol.hpp>

static ViSession s_rm = VI_NULL;
static ViSession s_session = VI_NULL;

static int plugin_init(FrasyPluginContext* ctx) {
    sol::state_view lua(ctx->lua);

    auto scpi = lua.create_named_table("Scpi");
    scpi["Open"] = [](const std::string& resource) -> int {
        viOpenDefaultRM(&s_rm);
        return viOpen(s_rm, resource.c_str(), VI_NULL, VI_NULL, &s_session);
    };
    scpi["Close"] = []() {
        if (s_session) viClose(s_session);
        if (s_rm) viClose(s_rm);
        s_session = VI_NULL;
        s_rm = VI_NULL;
    };
    scpi["Write"] = [](const std::string& cmd) -> int {
        ViUInt32 written;
        return viWrite(s_session, (ViBuf)cmd.c_str(), cmd.size(), &written);
    };
    scpi["Read"] = [](int maxBytes) -> std::string {
        std::string buf(maxBytes, '\0');
        ViUInt32 read;
        viRead(s_session, (ViBuf)buf.data(), maxBytes, &read);
        buf.resize(read);
        return buf;
    };

    return 0;
}

static void plugin_shutdown() {
    if (s_session) viClose(s_session);
    if (s_rm) viClose(s_rm);
}

static FrasyPluginInfo s_info {
    .abi_version            = FRASY_PLUGIN_ABI_VERSION,
    .name                   = "visa",
    .version                = "1.0.0",
    .description            = "SCPI/VISA instrument control",
    .init                   = plugin_init,
    .shutdown               = plugin_shutdown,
    .on_product_deselected  = nullptr,
    .on_render              = nullptr,
};

FRASY_PLUGIN_EXPORT FrasyPluginInfo* frasy_plugin_info() {
    return &s_info;
}
```

---

## Plugin Loading Flow

```
1. Environment.lua parsed → Environment.Plugin.Load("visa") encountered
2. Framework checks: is "visa" already loaded?
   - Yes → skip (differential loading)
   - No → continue
3. Framework searches plugins/ for visa.dll
4. LoadLibrary("plugins/visa.dll")
5. GetProcAddress → frasy_plugin_info()
6. Check abi_version == FRASY_PLUGIN_ABI_VERSION
   - Mismatch → error message, abort product load
7. Call info->init(&context)
8. Plugin is now active, its Lua functions are registered
```

---

## Plugin Lifecycle

| Event | What happens |
|-------|-------------|
| Product selected (needs plugin) | `init(ctx)` called if not already loaded |
| Product deselected (plugin loaded) | `on_product_deselected()` called (if non-null) |
| Product selected (plugin already loaded) | Nothing — plugin stays resident |
| Every frame (GUI mode) | `on_render()` called for all loaded plugins with non-null render |
| Application exit | `shutdown()` called for all loaded plugins |

Plugins are **never unloaded** during the session. They accumulate as products are switched. `shutdown()` is only called at application exit.

---

## Error Handling

| Condition | Error message |
|-----------|---------------|
| DLL not found | `"Product 'X' requires plugin 'visa' but plugins/visa.dll was not found"` |
| Missing export | `"plugins/visa.dll is not a valid Frasy plugin (missing frasy_plugin_info export)"` |
| ABI mismatch | `"Plugin 'visa' (ABI v2) is incompatible with this Frasy version (ABI v1). Recompile the plugin."` |
| init() failure | `"Plugin 'visa' failed to initialize"` |

---

## ABI Versioning

The `abi_version` field gates plugin compatibility:

| Scenario | Behavior |
|----------|----------|
| Plugin ABI == Frasy ABI | Loaded normally |
| Plugin ABI < Frasy ABI | Rejected with "plugin too old, recompile" message |
| Plugin ABI > Frasy ABI | Rejected with "plugin requires newer Frasy" message |

**When ABI version increments:**

- Changes to `FrasyPluginInfo` struct layout (reorder, remove, or change existing fields)
- Lua version change (e.g., Lua 5.4 → 5.5)
- Changes to `FrasyPluginContext` struct layout
- Calling convention changes

**When ABI version stays the same:**

- New fields added to the end of `FrasyPluginInfo` or `FrasyPluginContext` (with defaults)
- New Lua APIs added to the framework
- Bug fixes
- New framework features that don't change the plugin interface

---

## Plugin Template Project

Shipped as a minimal CMake project that plugin authors clone:

```
frasy-plugin-template/
    CMakeLists.txt          ← links frasy_plugin_sdk, vendor libs
    src/
        my_plugin.cpp       ← skeleton implementation
    include/
        frasy_plugin.h      ← from Frasy SDK
    vendor/                 ← place hardware vendor headers/libs here
    README.md               ← build, test, deploy instructions
```
