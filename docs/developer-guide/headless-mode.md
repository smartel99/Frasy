# Headless Mode — Developer Guide

This guide explains how to add headless and MCP support to your Frasy application.

---

## Overview

The Frasy framework provides headless execution at the framework level. To enable it in your application, you need to:

1. Implement a `ProductProvider` class
2. Register it in your application's Interpreter constructor
3. Optionally refactor your `makeOrchestrator` to use the same provider

---

## Implementing ProductProvider

Create a class that inherits from `Frasy::Headless::ProductProvider`:

```cpp
#include <utils/headless/product_provider.h>

class MyProductProvider : public Frasy::Headless::ProductProvider {
public:
    bool validateSerialNumber(const std::string& serial) override
    {
        // Validate serial format for your product
        // Return true if valid, false otherwise
        return serial.size() == 12 && serial.starts_with("SN");
    }

    bool setup(Frasy::Lua::Orchestrator& orchestrator,
               Frasy::CanOpen::CanOpen&   canOpen,
               const std::string&         product,
               const std::string&         envPath,
               const std::string&         testsDir) override
    {
        // 1. Select product-specific configuration
        selectProduct(product);

        // 2. Set Lua values (available at Context.values.gui)
        orchestrator.setLoadUserValues([this](sol::state_view lua) {
            return loadLuaValues(lua);
        });

        // 3. Load user files
        if (!orchestrator.loadUserFiles(envPath, testsDir)) {
            return false;
        }

        // 4. Configure CANopen
        canOpen.stop();
        canOpen.clearNodes();
        const auto& [ibs, uuts, teams] = orchestrator.getMap();
        for (const auto& ib : ibs | std::views::values) {
            canOpen.addNode(ib.nodeId, ib.name, ib.edsPath);
        }

        // 5. Start CANopen
        canOpen.start();

        // 6. Set Lua functions
        orchestrator.setLoadUserFunctions([this](sol::state_view lua) {
            loadLuaFunctions(lua);
        });

        return true;
    }

    void onTestComplete(Frasy::Lua::Orchestrator& orchestrator) override
    {
        // Optional: post-test actions (send reports, signal LEDs, etc.)
    }

private:
    void selectProduct(const std::string& product) { /* product routing */ }
    sol::table loadLuaValues(sol::state_view lua) { return lua.create_table(); }
    void loadLuaFunctions(sol::state_view lua) { /* register custom Lua functions */ }
};
```

### Method Responsibilities

#### `validateSerialNumber(serial)`

Called before tests start for each serial provided via `--serial` or `run_tests`. Return `false` to reject invalid serials (headless mode exits with code 2, MCP returns an error).

#### `setup(orchestrator, canOpen, product, envPath, testsDir)`

Called once before test execution. Must configure the orchestrator and hardware for the given product. This is the **single source of truth** — both the GUI path and headless/MCP paths call this method.

The order of operations matters:

1. Select product-specific config (routing logic)
2. `orchestrator.setLoadUserValues(...)` — before loadUserFiles
3. `orchestrator.loadUserFiles(envPath, testsDir)` — loads environment.lua and test files
4. Configure CANopen nodes from the IB map
5. `canOpen.start()`
6. `orchestrator.setLoadUserFunctions(...)` — after loadUserFiles

#### `onTestComplete(orchestrator)`

Called after the test run finishes (regardless of pass/fail). Use it for:

- Sending reports via email
- Setting signaling LEDs
- Logging to external systems

---

## Registering the Provider

In your application's Interpreter constructor, register the provider **before** pushing the main layer:

```cpp
class MyInterpreter : public Frasy::Interpreter {
public:
    MyInterpreter() : Interpreter("My App")
    {
        setProductProvider(std::make_unique<MyProductProvider>());
        pushLayer(new MyMainApplicationLayer());
    }
};
```

The provider is accessible from anywhere via:

```cpp
auto* provider = Frasy::Interpreter::Get().getProductProvider();
```

---

## Refactoring makeOrchestrator

Your GUI layer's `makeOrchestrator` can now delegate to the same provider:

### Before

```cpp
void MyLayer::makeOrchestrator(const std::string& name,
                               const std::string& envPath,
                               const std::string& testPath) {
    // Duplicated setup logic...
    if (m_orchestrator.loadUserFiles(envPath, testPath)) {
        m_canOpen.stop();
        m_canOpen.clearNodes();
        // ... configure CANopen ...
        m_canOpen.start();
        m_orchestrator.setLoadUserFunctions([&](auto lua) { ... });
        // GUI-specific setup
        m_activeProduct = name;
        m_serials.resize(uuts.size() + 1);
    } else {
        Brigerad::warningDialog("Frasy", "Failed!");
    }
}
```

### After

```cpp
void MyLayer::makeOrchestrator(const std::string& name,
                               const std::string& envPath,
                               const std::string& testPath) {
    auto* provider = Frasy::Interpreter::Get().getProductProvider();
    if (provider && provider->setup(m_orchestrator, m_canOpen, name, envPath, testPath)) {
        // GUI-specific setup only
        m_activeProduct = name;
        const auto& [ibs, uuts, teams] = m_orchestrator.getMap();
        m_serials.resize(uuts.size() + 1);
    } else {
        Brigerad::warningDialog("Frasy", "Failed!");
    }
}
```

---

## Product Routing

If your application has multiple products with different configurations, implement the routing in `setup()`:

```cpp
bool setup(Orchestrator& orchestrator, CanOpen& canOpen,
           const std::string& product, const std::string& envPath,
           const std::string& testsDir) override
{
    // Select product-specific behavior
    if (product == "product_a") {
        orchestrator.setLoadUserValues([](sol::state_view lua) {
            auto t = lua.create_table();
            t["voltage"] = 24.0;
            return t;
        });
    } else if (product == "product_b") {
        orchestrator.setLoadUserValues([](sol::state_view lua) {
            auto t = lua.create_table();
            t["voltage"] = 12.0;
            return t;
        });
    }

    // Common setup
    if (!orchestrator.loadUserFiles(envPath, testsDir)) return false;
    canOpen.stop();
    canOpen.clearNodes();
    const auto& [ibs, uuts, teams] = orchestrator.getMap();
    for (const auto& ib : ibs | std::views::values) {
        canOpen.addNode(ib.nodeId, ib.name, ib.edsPath);
    }
    canOpen.start();
    return true;
}
```

---

## Testing Headless Mode

### Quick test from command line

```bash
frasy.exe --headless --product MyProduct --operator Test --serial SN001 --skip-verification
```

### With verbose logging

```bash
frasy.exe --headless --product MyProduct --operator Test --serial SN001 --verbose
```

### JSON output for scripting

```bash
frasy.exe --headless --product MyProduct --operator Test --serial SN001 --output-format json
```

### MCP mode testing

Configure as an MCP server in your Kiro agent and use the tools interactively:

```json
{
  "mcpServers": {
    "frasy": {
      "command": "cmd",
      "args": ["/c", "cd /d C:\\path\\to\\bin && frasy.exe --mcp-server"]
    }
  }
}
```

Then use `list_products`, `run_tests`, `get_status`, `get_pending_popup`, `respond_to_popup`, and `get_results` tools from the agent.

---

## Embedded MCP HTTP Server

### How It Works

When `--mcp-port <port>` is passed, `MainApplicationLayer::onAttach()` creates a `McpHttpServerLayer` that:

1. Starts an HTTP server on a background thread (using cpp-httplib)
2. Registers the same MCP tools as the standalone `--mcp-server` mode
3. Shares the GUI's `m_orchestrator` and `m_canOpen` — no separate instances
4. Processes mutating operations (run_tests, load_product) via a command queue drained each frame on the main thread
5. Serves read-only operations (get_status, list_nodes, upload_sdo) directly from the HTTP handler threads

### Thread Safety Model

```
HTTP handler threads                Main thread (GUI)
─────────────────────              ─────────────────────
get_status()         → direct      onUpdate() drains:
list_nodes()         → direct        - load_product commands
upload_sdo()         → direct        - run_tests commands
get_pending_popup()  → direct        - popup sync
respond_to_popup()   → direct
load_product()       → enqueue ─→  main thread executes
run_tests()          → enqueue ─→  main thread executes
```

### Popup Dual-Path

The `UnifiedPopupHandler` observes the orchestrator's popup map each frame:

- Detects new popups → makes them available via `get_pending_popup`
- Detects consumed popups (GUI operator responded) → cleans up tracking
- `respond_to_popup` sets inputs and calls `Popup::clickButton()` under the popup mutex

Both the ImGui rendering and the MCP handler can consume the same popup. The `Popup::m_consumed` atomic ensures first-responder-wins semantics.

### Adding the MCP Server to Your Application

The framework handles this automatically. If you've registered a `ProductProvider` (which you already do for headless mode), the MCP server will work out of the box with `--mcp-port`:

```cpp
// No changes needed — MainApplicationLayer does this for you:
if (CliArgs::get().mcpPort >= 0) {
    m_mcpHttpServer = std::make_unique<McpHttpServerLayer>(
      port, m_orchestrator, m_canOpen, *provider);
    m_mcpHttpServer->setProductCallbacks(
      [this]() { return getActiveProduct(); },
      [this](const std::string& name) { return loadProduct(name); });
    m_mcpHttpServer->onAttach();
}
```

To make `load_product` work from MCP, override `getActiveProduct()` and `loadProduct()` in your derived layer:

```cpp
class MyMainApplicationLayer : public Frasy::MainApplicationLayer {
protected:
    std::string getActiveProduct() const override { return m_activeProduct; }
    bool loadProduct(const std::string& name) override {
        // Find product, set m_activeProduct, call your existing loadProduct()
        m_activeProduct = name;
        loadProduct();  // your no-arg version
        return !m_orchestrator.getMap().uuts.empty();
    }
};
```

### Testing the Embedded MCP Server

```bash
# Start with MCP server
frasy.exe --mcp-port 8080

# In another terminal, test with curl:
curl -X POST http://127.0.0.1:8080/mcp \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}'

# List products:
curl -X POST http://127.0.0.1:8080/mcp \
  -H "Content-Type: application/json" \
  -H "Mcp-Session-Id: <session-id-from-initialize>" \
  -d '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"list_products","arguments":{}}}'
```

### Testing the MCP Client Relay

```bash
# Start the GUI with MCP server
frasy.exe --mcp-port 8080

# In another terminal, start the relay
frasy.exe --mcp-client --port 8080

# The relay reads from stdin and writes to stdout — pipe JSON-RPC:
echo {"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}} | frasy.exe --mcp-client --port 8080
```

---

## RunOwner

The `RunOwner` enum (`Frasy/src/utils/run_owner.h`) tracks who initiated the current test run:

```cpp
enum class RunOwner { None, Gui, Mcp };
```

When `run_tests` is called via MCP, the layer sets `RunOwner::Mcp`. Your GUI layer should set `RunOwner::Gui` when the operator clicks Run. Both sides check `isRunning()` before starting — first caller wins.

The `get_status` tool response includes an `"initiated_by"` field (`"gui"`, `"mcp"`, or `"none"`) so agents can understand who is controlling the current run.
