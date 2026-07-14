# C++ Application Setup

This page explains how to set up the C++ side of a Frasy application. You need two classes:

1. An **Interpreter** subclass — the application entry point
2. A **MainApplicationLayer** subclass — the operator-facing UI and test orchestration logic

---

## Entry Point: Subclassing `Frasy::Interpreter`

`Frasy::Interpreter` is the application singleton. It owns `config.json` and the serial device map.
Your subclass is created by `Brigerad::CreateApplication()`, which Brigerad calls at startup.

A minimal entry point looks like this:

```cpp
// src/my_frasy_interpreter.cpp
#include "layers/my_main_application_layer.h"
#include <Brigerad.h>
#include <Brigerad/Core/EntryPoint.h>
#include <frasy_interpreter.h>

class MyFrasyInterpreter : public Frasy::Interpreter {
public:
    MyFrasyInterpreter() : Interpreter("My App Title")
    {
        pushLayer(new MyMainApplicationLayer());
    }
};

Brigerad::Application* Brigerad::CreateApplication(int argc, char** argv)
{
    return new MyFrasyInterpreter();
}
```

The constructor should:

1. Call `Interpreter("window title")` — this loads `config.json` and initializes the device map.
2. Push your main layer onto the layer stack with `pushLayer()`.

`Frasy::Interpreter` owns a `nlohmann::json` member called `m_config`. It is loaded from
`config.json` (located next to the executable) at construction time, and saved back to disk when
the application exits. This is the application's persistent configuration store — it holds
settings like the last selected product, USB device whitelist, logging preferences, and any
custom sections you define.

You can initialize your own config sections in the constructor to ensure they exist at runtime:

```cpp
MyFrasyInterpreter() : Interpreter("My App Title")
{
    if (m_config["MySection"].empty()) {
        m_config["MySection"] = nlohmann::json::object();
    }
    pushLayer(new MyMainApplicationLayer());
}
```

Elsewhere in the application, access the config through `Frasy::Interpreter::Get().getConfig()`.

---

## Main Layer: Subclassing `Frasy::MainApplicationLayer`

`MainApplicationLayer` is where all the action happens. It owns the built-in UI panels, the
test orchestrator (`m_orchestrator`), and the CANopen bus (`m_canOpen`). Your subclass overrides
a few methods to build the operator UI and wire up test execution.

### Header

```cpp
// src/layers/my_main_application_layer.h
#ifndef MY_MAIN_APPLICATION_LAYER_H
#define MY_MAIN_APPLICATION_LAYER_H

#include <layers/main_application_layer.h>
#include <utils/lua/orchestrator/orchestrator.h>

class MyMainApplicationLayer final : public Frasy::MainApplicationLayer {
public:
    ~MyMainApplicationLayer() override = default;

    void onAttach() override;
    void onUpdate(Brigerad::Timestep ts) override;

protected:
    void renderControlRoom() override;

private:
    // Your application state here
};

#endif // MY_MAIN_APPLICATION_LAYER_H
```

### Key Overrides

#### `onAttach()`

Called once when the layer is pushed. Use it to discover available products and select the
initial one:

```cpp
void MyMainApplicationLayer::onAttach()
{
    MainApplicationLayer::onAttach();  // Always call the base — it loads textures and panels
    loadProducts();
}
```

#### `onUpdate(Brigerad::Timestep ts)`

Called every frame before rendering. Use it for hotkeys or periodic checks:

```cpp
void MyMainApplicationLayer::onUpdate(Brigerad::Timestep ts)
{
    MainApplicationLayer::onUpdate(ts);  // Always call the base
    if (Brigerad::Input::isKeyPressed(Brigerad::KeyCode::F9)) { loadProducts(); }
}
```

#### `renderControlRoom()`

This is the main override point. It renders the operator-facing UI panel — typically a product
selector, operator name field, serial number input, and a run button.

```cpp
void MyMainApplicationLayer::renderControlRoom()
{
    ImGui::Begin("Control Room");

    // Product selector
    if (ImGui::BeginCombo("##Product", m_activeProduct.c_str())) {
        for (auto& product : m_products) {
            if (ImGui::Selectable(product.name.c_str(), product.name == m_activeProduct)) {
                makeOrchestrator(product.name, product.environmentPath, product.testPath);
            }
        }
        ImGui::EndCombo();
    }

    // Run button
    if (ImGui::Button("Run") && !m_orchestrator.isRunning()) {
        m_orchestrator.runSolution(operator, serials, shouldRegenerate, skipVerification, callback);
    }

    ImGui::End();
}
```

---

## Product Discovery and the Orchestrator

A **product** is a directory under `lua/user/` containing an `environment.lua` and a `tests/`
folder. Your layer needs to scan for products and feed them to the orchestrator.

### Detecting Products

Scan `lua/user/` for subdirectories containing `environment.lua`:

```cpp
std::vector<ProductInfo> MyMainApplicationLayer::detectProducts()
{
    std::vector<ProductInfo> products;

    for (const auto& entry : std::filesystem::recursive_directory_iterator("lua/user")) {
        if (!entry.is_directory()) continue;

        auto environment = entry.path() / "environment.lua";
        if (!std::filesystem::exists(environment)) continue;

        auto envPath = environment;
        envPath.replace_extension();  // Remove .lua extension for the orchestrator

        products.push_back({
            .environmentPath = envPath.string(),
            .testPath        = entry.path().string(),
            .name            = entry.path().filename().string(),
        });
    }

    return products;
}
```

### Loading a Product into the Orchestrator

Use `m_orchestrator.loadUserFiles()` to load a product's environment and test files. On success,
configure CANopen nodes from the environment's IB declarations:

```cpp
void MyMainApplicationLayer::makeOrchestrator(const std::string& name,
                                              const std::string& envPath,
                                              const std::string& testPath)
{
    if (m_orchestrator.loadUserFiles(envPath, testPath)) {
        m_canOpen.stop();
        m_canOpen.clearNodes();
        m_activeProduct = name;

        const auto& [ibs, uuts, teams] = m_orchestrator.getMap();
        for (const auto& [kind, nodeId, ibName, edsPath, od] : ibs | std::views::values) {
            m_canOpen.addNode(static_cast<uint8_t>(nodeId), ibName, edsPath);
        }
        m_canOpen.start();

        // Register custom Lua bindings
        m_orchestrator.setLoadUserFunctions([&](const sol::state_view& lua) {
            loadLuaFunctions(lua);
        });
    }
    else {
        BR_LOG_ERROR("APP", "Unable to initialize orchestrator!");
    }
}
```

---

## Running Tests

To execute the test solution, call `m_orchestrator.runSolution()`:

```cpp
m_orchestrator.runSolution(
    operatorName,       // std::string — who is running the test
    serialNumbers,      // std::vector<std::string> — one serial per UUT
    shouldRegenerate,   // bool — true if Lua files changed since last generation
    skipVerification,   // bool — skip SHA hash verification (debug only)
    [this] { onDone(); } // callback when execution finishes
);
```

The orchestrator runs asynchronously. Use `m_orchestrator.isRunning()` to check status, and
query UUT states with `m_orchestrator.getUutState(uut)`.

---

## Exposing Custom C++ Functions to Lua

Register a callback via `m_orchestrator.setLoadUserFunctions()` to bind your own C++ functions
into the Lua environment. These become available to all test scripts:

```cpp
void MyMainApplicationLayer::loadLuaFunctions(sol::state_view lua)
{
    lua["MyCustomFunction"] = [](int value) {
        // Your hardware interaction code
        return value * 2;
    };
}
```

This is called during the generation stage, before any test files are executed.

---

## Protected Members Available from `MainApplicationLayer`

Your subclass has access to these members:

| Member | Type | Purpose |
|---|---|---|
| `m_orchestrator` | `Lua::Orchestrator` | The test engine — load products, run solutions, query state |
| `m_canOpen` | `CanOpen::CanOpen` | CANopen bus — add/remove nodes, start/stop communication |
| `m_logWindow` | `unique_ptr<LogWindow>` | Built-in log panel |
| `m_resultViewer` | `unique_ptr<ResultViewer>` | Built-in result panel |
| `m_resultAnalyzer` | `unique_ptr<ResultAnalyzer>` | Built-in statistics panel |
| `m_testViewer` | `unique_ptr<TestViewer>` | Built-in test explorer panel |
| `m_deviceViewer` | `unique_ptr<DeviceViewer>` | Built-in USB device panel |
| `m_canOpenViewer` | `unique_ptr<CanOpenViewer::Layer>` | Built-in OD browser panel |
| `m_run`, `m_pass`, `m_fail`, ... | `Ref<Texture2D>` | Icon textures for UUT states |

### Helper Methods

| Method | Purpose |
|---|---|
| `makeLogWindowVisible()` | Open the log panel programmatically |
| `makeResultViewerVisible()` | Open the result viewer |
| `makeTestViewerVisible()` | Open the test viewer |
| `appendToMainTabBar()` | Override to add items to the top menu bar |

---

## Minimal Complete Example

Putting it all together, the simplest functional application needs:

1. `src/my_frasy_interpreter.cpp` — creates the interpreter and pushes the layer
2. `src/layers/my_main_application_layer.h` — declares your layer subclass
3. `src/layers/my_main_application_layer.cpp` — implements product loading, UI, and test execution
4. `src/lua/user/<product>/environment.lua` — at least one product
5. `src/lua/user/<product>/tests/*.lua` — at least one test file

The template repository provides all of these as a working starting point.

---

## Next Steps

- [Lua Setup](lua-setup.md) — environment configuration and writing test sequences
- [Customizing the UI](../developer-guide/customizing-the-ui.md) — go beyond the basics
- [Exposing C++ to Lua](../developer-guide/cpp-to-lua.md) — bind hardware drivers into the scripting layer
