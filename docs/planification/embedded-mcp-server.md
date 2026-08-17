# Implementation Plan — Embedded MCP Server in GUI

## Problem Statement

The current MCP server mode (`--mcp-server`) runs as a separate execution path that creates its own `CanOpen::CanOpen` and `Lua::Orchestrator` instances. Since only one process can have USB SLCAN devices open at a time, this makes it impossible to use the MCP server while the GUI is running — the two paths fight over USB device access.

**Goal:** Embed the MCP server inside the GUI process so that a single Frasy instance can simultaneously display the GUI for operators AND serve MCP tool calls for AI agents — sharing the same `CanOpen` and `Orchestrator` instances. The operator can follow test progress in real-time regardless of whether the run was triggered from the GUI or by an AI agent.

---

## Key Design Decisions

1. **Shared orchestrator and CanOpen** — The GUI and MCP server share the exact same instances. When the MCP agent starts a run, the operator sees progress in the GUI panels (test viewer, result viewer, profiler, popups). When the operator starts a run, the MCP agent can query status and results.

2. **Dual-path popup handling from day one** — Popups are visible both in the GUI (ImGui) and to the MCP agent (`get_pending_popup`). Either the operator or the agent can respond; first responder wins.

3. **MCP over Streamable HTTP** — The GUI process hosts an HTTP server on a configurable port. A separate process (or remote machine) can connect to it. This also enables a "client mode" where a second Frasy instance connects to the first one's HTTP endpoint.

4. **`--mcp-client` for remote connection** — `frasy.exe --mcp-client --address 192.168.0.105 --port 69` launches a lightweight relay process that connects to a running Frasy instance's HTTP endpoint and bridges it over stdio for AI agents.

---

## Architecture

### Target State

```
┌───────────────────────────────────────────────────────────────┐
│ frasy.exe (Primary Instance — GUI + MCP HTTP Server)          │
│                                                               │
│  Main Thread (GUI + Brigerad loop)                            │
│  ┌──────────────────────────────────┐                         │
│  │ Application::run()              │                         │
│  │  → onUpdate() / onImGuiRender()│                         │
│  │  → renderPopups() (dual-path)  │                         │
│  │  → process MCP command queue   │                         │
│  └──────────────────────────────────┘                         │
│                                                               │
│  HTTP Server Thread (background)                              │
│  ┌──────────────────────────────────┐                         │
│  │ httplib::Server on port N        │                         │
│  │  POST /mcp → JSON-RPC dispatch  │                         │
│  │  GET  /mcp → SSE stream         │                         │
│  └──────────────────────────────────┘                         │
│                                                               │
│  Shared State (owned by MainApplicationLayer)                 │
│  ┌──────────────────────────────────┐                         │
│  │ m_canOpen ────────────── ← USB  │                         │
│  │ m_orchestrator                  │                         │
│  └──────────────────────────────────┘                         │
│                                                               │
│  Lua Worker Threads (spawned by orchestrator)                 │
│  ┌──────────────────────────────────┐                         │
│  │ Test execution, popup blocking  │                         │
│  └──────────────────────────────────┘                         │
└───────────────────────────────────────────────────────────────┘
         ↕ USB (single owner)               ↕ HTTP (port N)
    ┌──────────┐                   ┌─────────────────────────┐
    │ Hardware │                   │ Remote MCP Clients       │
    └──────────┘                   │  - AI agent (Kiro)       │
                                   │  - frasy --mcp-client    │
                                   │    --address x --port N  │
                                   └─────────────────────────┘
```

### Remote Client Mode

```
┌────────────────────────────────────────┐
│ frasy.exe --mcp-client                 │
│          --address 192.168.0.105       │
│          --port 69                     │
│                                        │
│  stdio ←→ MCP-over-HTTP bridge         │
│                                        │
│  AI Agent (Kiro) connects via stdio    │
│  This process connects to the primary  │
│  instance's HTTP endpoint and relays   │
│  JSON-RPC messages.                    │
└────────────────────────────────────────┘
         ↕ stdio (AI agent)
    ┌──────────┐
    │ Kiro CLI │
    └──────────┘
```

---

## Launch Modes

| Command | Behavior |
|---------|----------|
| `frasy.exe` | Normal GUI, no MCP server |
| `frasy.exe --mcp-port 8080` | GUI + MCP HTTP server on port 8080 |
| `frasy.exe --mcp-port 0` | GUI + MCP HTTP server on random available port (printed to stderr) |
| `frasy.exe --mcp-server` | Standalone MCP over stdio (existing behavior, own orchestrator — **deprecated but kept for compat**) |
| `frasy.exe --mcp-client --port 8080` | Client/relay mode: bridges stdio ↔ HTTP to a running primary instance on localhost |
| `frasy.exe --mcp-client --address 192.168.0.105 --port 69` | Client/relay mode: bridges stdio ↔ HTTP to a remote primary instance |
| `frasy.exe --headless ...` | Headless CLI mode (existing, unchanged) |

**Kiro agent configurations:**

```jsonc
// Option A: Direct stdio connection (existing standalone mode, deprecated)
{
  "mcpServers": {
    "frasy": {
      "command": "cmd",
      "args": ["/c", "cd /d C:\\path\\to\\bin && frasy.exe --mcp-server"]
    }
  }
}

// Option B: Connect to an already-running GUI instance via relay
{
  "mcpServers": {
    "frasy": {
      "command": "cmd",
      "args": ["/c", "cd /d C:\\path\\to\\bin && frasy.exe --mcp-client --port 8080"]
    }
  }
}

// Option C: Connect to a remote instance
{
  "mcpServers": {
    "frasy": {
      "command": "cmd",
      "args": ["/c", "cd /d C:\\path\\to\\bin && frasy.exe --mcp-client --address 192.168.0.105 --port 69"]
    }
  }
}

// Option D: Streamable HTTP (if the MCP client supports it natively)
{
  "mcpServers": {
    "frasy": {
      "type": "streamable-http",
      "url": "http://127.0.0.1:8080/mcp"
    }
  }
}
```

---

## Transport: MCP Streamable HTTP

The MCP spec (2025-03-26) defines Streamable HTTP as the standard transport for remote connections. It uses a single HTTP endpoint (`/mcp`) that supports:

- **POST /mcp** — Client sends JSON-RPC requests/notifications. Server responds with either `application/json` (single response) or `text/event-stream` (SSE stream for streaming responses + server-initiated messages).
- **GET /mcp** — Client opens an SSE stream for server-to-client notifications (used for progress events, popup notifications).

### Why HTTP over Stdio for the embedded case

| Concern | Stdio | HTTP |
|---------|-------|------|
| Multiple clients | ❌ Single | ✅ Multiple |
| GUI app on Windows | ❌ Needs console alloc | ✅ No console needed |
| Remote connections | ❌ Local only | ✅ Network accessible |
| Runtime toggle | ❌ Fixed at launch | ✅ Can start/stop server |
| MCP spec support | ✅ Universal | ✅ Streamable HTTP spec |
| Dependency | None | cpp-httplib (header-only) |

### HTTP Library: cpp-httplib

[cpp-httplib](https://github.com/yhirose/cpp-httplib) — single-header, C++11, MIT license. It provides:
- Blocking HTTP server with thread pool
- SSE support via chunked transfer encoding
- No external dependencies (no OpenSSL needed for localhost)
- Already proven in similar embedded scenarios

---

## Task Breakdown

### Task 1: Add cpp-httplib Dependency

**Objective:** Add the `cpp-httplib` header-only library to the vendor tree.

**Implementation:**
1. Add `httplib.h` to `Frasy/vendor/cpp-httplib/` (or as a git submodule).
2. Add to CMakeLists.txt as an interface library.
3. Verify it compiles with the project's MSVC C++23 settings.

**Changes:**
- `Frasy/vendor/cpp-httplib/httplib.h` (single header, ~30KB)
- `Frasy/CMakeLists.txt` — add include path

---

### Task 2: CLI Flags

**Objective:** Add `--mcp-port`, `--mcp-client`, `--address`, and `--port` flags to `CliArgs`.

**New flags:**

| Flag | Description | Default |
|------|-------------|---------|
| `--mcp-port <port>` | Start HTTP MCP server on this port (0 = random) | Not set (no server) |
| `--mcp-client` | Run as a stdio ↔ HTTP relay connecting to a primary instance | false |
| `--address <addr>` | For `--mcp-client`: address of the primary instance | `127.0.0.1` |
| `--port <port>` | For `--mcp-client`: port of the primary instance | Required with `--mcp-client` |

**Behavioral logic in entry point:**

```cpp
if (cliArgs.mcpClient) {
    // Client/relay mode: bridge stdio ↔ HTTP
    Mcp::McpRelay relay(cliArgs.address, cliArgs.port);
    return relay.run();
}
else if (cliArgs.mcpServer) {
    // Standalone mode (deprecated, backward compat)
    // ... existing behavior ...
}
else {
    // GUI mode
    app->run();
    // If --mcp-port was specified, McpHttpServerLayer starts the HTTP server
}
```

**Validation:**
- `--mcp-port` can only be used in GUI mode (error if combined with `--headless`, `--mcp-server`, or `--mcp-client`).
- `--mcp-client` requires `--port`. `--address` defaults to `127.0.0.1` if not specified.
- `--mcp-client` is mutually exclusive with `--mcp-server`, `--headless`, and `--mcp-port`.
- `--mcp-server` (standalone, deprecated) still works exactly as before when used alone.

**Changes:**
- `Frasy/src/utils/cli/cli_args.h` — add `mcpClient`, `address`, `port` fields
- `Brigerad/src/Brigerad/Core/EntryPoint.h` — branching logic for `--mcp-client`

---

### Task 3: MCP HTTP Server Layer

**Objective:** Create `McpHttpServerLayer` — a Brigerad Layer that hosts the HTTP server and dispatches MCP tool calls using the shared orchestrator and CanOpen.

**Implementation:**

```cpp
// Frasy/src/layers/mcp_http_server_layer.h
namespace Frasy {
class McpHttpServerLayer : public Brigerad::Layer {
public:
    McpHttpServerLayer(int port,
                       Lua::Orchestrator& orchestrator,
                       CanOpen::CanOpen& canOpen,
                       Headless::ProductProvider& provider);

    void onAttach() override;   // Start HTTP server thread
    void onDetach() override;   // Stop HTTP server
    void onUpdate(Brigerad::Timestep ts) override;  // Process command queue + sync popups

private:
    // HTTP handlers
    void handlePost(const httplib::Request& req, httplib::Response& res);
    void handleGet(const httplib::Request& req, httplib::Response& res);

    // JSON-RPC dispatch (reused from existing McpServer logic)
    nlohmann::json dispatch(const nlohmann::json& message);

    // Tool handlers (same interface as McpRunner)
    void registerTools();
    nlohmann::json handleListProducts(const nlohmann::json& args);
    nlohmann::json handleLoadProduct(const nlohmann::json& args);
    nlohmann::json handleRunTests(const nlohmann::json& args);
    nlohmann::json handleGetStatus(const nlohmann::json& args);
    nlohmann::json handleGetResults(const nlohmann::json& args);
    nlohmann::json handleGetPendingPopup(const nlohmann::json& args);
    nlohmann::json handleRespondToPopup(const nlohmann::json& args);
    nlohmann::json handleListNodes(const nlohmann::json& args);
    nlohmann::json handleListDevices(const nlohmann::json& args);
    nlohmann::json handleUploadSdo(const nlohmann::json& args);
    nlohmann::json handleDownloadSdo(const nlohmann::json& args);

    // Command queue for main-thread execution
    struct Command {
        std::function<nlohmann::json()> execute;
        std::promise<nlohmann::json> result;
    };
    std::mutex m_commandMutex;
    std::queue<Command> m_commandQueue;

    nlohmann::json enqueueAndWait(std::function<nlohmann::json()> fn);

    // Server
    httplib::Server m_httpServer;
    std::jthread m_serverThread;
    int m_port;

    // Shared state
    Lua::Orchestrator& m_orchestrator;
    CanOpen::CanOpen& m_canOpen;
    Headless::ProductProvider& m_provider;

    // Popup dual-path handler
    Mcp::UnifiedPopupHandler m_popupHandler;

    // Session management
    std::string m_sessionId;

    // Run ownership
    std::atomic<RunOwner> m_runOwner = RunOwner::None;
};
}
```

**HTTP endpoint behavior (per MCP Streamable HTTP spec):**

- `POST /mcp`:
  - Accepts JSON-RPC request body.
  - Returns `Content-Type: application/json` with the JSON-RPC response.
  - For `initialize`: also sets `Mcp-Session-Id` response header.
  - Validates `Mcp-Session-Id` header on subsequent requests.
  - Validates `Origin` header to prevent DNS rebinding (accept only localhost origins or configured origins).

- `GET /mcp`:
  - Opens SSE stream for server-to-client notifications.
  - Used to push progress events and popup notifications to the MCP client proactively.
  - Returns `Content-Type: text/event-stream`.

- `DELETE /mcp`:
  - Client terminates the session.
  - Server cleans up session state.

**Changes:**
- New: `Frasy/src/layers/mcp_http_server_layer.h/.cpp`
- New: `Frasy/src/utils/mcp/mcp_http_dispatch.h/.cpp` (shared JSON-RPC dispatch logic)

---

### Task 4: Command Queue for Thread-Safe Mutating Operations

**Objective:** HTTP server handler threads call mutating orchestrator methods indirectly via a command queue that the main thread processes each frame.

**Implementation:**

```cpp
nlohmann::json McpHttpServerLayer::enqueueAndWait(std::function<nlohmann::json()> fn)
{
    Command cmd;
    cmd.execute = std::move(fn);
    auto future = cmd.result.get_future();
    {
        std::lock_guard lock(m_commandMutex);
        m_commandQueue.push(std::move(cmd));
    }
    return future.get();  // Block HTTP handler thread until main thread executes
}

void McpHttpServerLayer::onUpdate(Brigerad::Timestep ts)
{
    // Drain command queue on main thread
    std::lock_guard lock(m_commandMutex);
    while (!m_commandQueue.empty()) {
        auto cmd = std::move(m_commandQueue.front());
        m_commandQueue.pop();
        try {
            cmd.result.set_value(cmd.execute());
        } catch (const std::exception& e) {
            cmd.result.set_value({{"error", e.what()}});
        }
    }

    // Sync popup state (Task 5)
    m_popupHandler.sync();
}
```

**Classification of operations:**

| Operation | Thread Safety | Execution |
|-----------|-------------|-----------|
| `list_products` | Read-only | Direct on HTTP thread |
| `get_status` | Read-only (atomics/futures) | Direct on HTTP thread |
| `get_results` | Read-only (file reads) | Direct on HTTP thread |
| `list_nodes` | Read-only | Direct on HTTP thread |
| `list_devices` | Read-only | Direct on HTTP thread |
| `get_pending_popup` | Read (mutex-protected queue) | Direct on HTTP thread |
| `upload_sdo` | Thread-safe (SDO queue) | Direct on HTTP thread |
| `download_sdo` | Thread-safe (SDO queue) | Direct on HTTP thread |
| `respond_to_popup` | Sets data + notifies CV | Direct on HTTP thread |
| `load_product` | **Mutates** orchestrator | **Command queue → main thread** |
| `run_tests` | **Mutates** orchestrator | **Command queue → main thread** |
| `set_test_enable` | **Mutates** solution | **Command queue → main thread** |

**Changes:**
- Part of `Frasy/src/layers/mcp_http_server_layer.cpp`

---

### Task 5: Dual-Path Unified Popup Handler

**Objective:** Popups triggered during test execution are visible simultaneously in the GUI (ImGui) and to the MCP agent. Either can respond; first responder wins.

**How the existing GUI popup system works:**

1. Lua worker calls `__popup.Show(builder)` → creates a `Popup` object, inserts into `m_popups` map (under `m_popupMutex`).
2. Worker calls `popup.Routine()` which **blocks** — it loops calling `std::this_thread::yield()` checking `m_consumed`.
3. Main thread calls `orchestrator.renderPopups()` each frame → renders ImGui, including buttons.
4. Button click sets `m_consumed = true` → Routine() exits → worker continues.
5. Worker reads inputs from the `Popup`, removes it from the map.

**How the MCP popup system works (standalone):**

1. Custom `importPopup` overrides the default → creates a `PendingPopup` with condition_variable.
2. Worker blocks on `cv.wait()`.
3. MCP client calls `respond_to_popup` → fills response, notifies CV.
4. Worker wakes, reads response.

**Unified approach — Observer pattern on the existing GUI popups:**

Keep the existing GUI popup system (it works). Layer an MCP observation mechanism on top:

```cpp
// Frasy/src/utils/mcp/unified_popup_handler.h
namespace Frasy::Mcp {

class UnifiedPopupHandler {
public:
    /// Register a new popup that just appeared in the orchestrator's popup map.
    /// Called when we detect a new popup during sync().
    void trackPopup(const std::string& id, std::size_t uut,
                    const Lua::Popup& popup);

    /// Remove tracking for a popup that was consumed (by GUI or MCP).
    void untrackPopup(const std::string& id);

    /// Called each frame from onUpdate(). Detects new/consumed popups.
    void sync(Lua::Orchestrator& orchestrator);

    /// MCP: get the next pending popup as JSON.
    std::optional<nlohmann::json> getPendingPopup();

    /// MCP: respond to a popup.
    /// Sets input values on the Popup object and calls Consume().
    bool respondToPopup(const std::string& id,
                        const std::map<int, std::string>& inputs,
                        const std::string& button,
                        Lua::Orchestrator& orchestrator);

private:
    struct TrackedPopup {
        std::string id;
        std::size_t uut;
        std::vector<std::string> texts;
        std::vector<std::string> inputTitles;
        std::vector<std::string> buttons;
        bool consumed = false;
    };

    std::mutex m_mutex;
    std::vector<TrackedPopup> m_tracked;
    std::set<std::string> m_knownPopups;  // IDs we've already seen
};

}
```

**sync() logic (called every frame on main thread):**

```cpp
void UnifiedPopupHandler::sync(Lua::Orchestrator& orchestrator)
{
    std::lock_guard popupLock(*orchestrator.getPopupMutex());
    auto& popups = orchestrator.getPopups();  // Need to expose this

    std::lock_guard lock(m_mutex);

    // Detect new popups
    for (auto& [name, popup] : popups) {
        if (!m_knownPopups.contains(name)) {
            m_knownPopups.insert(name);
            TrackedPopup tracked;
            tracked.id = name;
            // Extract text/input/button info from popup elements...
            m_tracked.push_back(std::move(tracked));
        }
    }

    // Detect consumed popups (GUI operator responded)
    std::erase_if(m_tracked, [&](const TrackedPopup& tp) {
        return !popups.contains(tp.id);  // Popup removed from map = consumed
    });

    // Clean known set
    std::erase_if(m_knownPopups, [&](const std::string& id) {
        return !popups.contains(id);
    });
}
```

**respondToPopup() logic (called from HTTP handler thread):**

```cpp
bool UnifiedPopupHandler::respondToPopup(
    const std::string& id,
    const std::map<int, std::string>& inputs,
    const std::string& button,
    Lua::Orchestrator& orchestrator)
{
    // This needs to:
    // 1. Find the Popup in orchestrator's m_popups map
    // 2. Set the input values on the Popup's Input elements
    // 3. Invoke the button action (if any)
    // 4. Call Consume() on the popup

    // Since this touches the Popup object (which the Lua worker is blocking on),
    // we need the popup mutex. The worker is in Routine() yielding,
    // so accessing the popup is safe under the mutex.

    std::lock_guard popupLock(*orchestrator.getPopupMutex());
    auto& popups = orchestrator.getPopups();
    auto it = popups.find(id);
    if (it == popups.end()) return false;

    auto& popup = it->second;

    // Set inputs
    // ... access popup elements, find Input elements, set buffer values ...

    // Click the button (call action + consume)
    // ... find Button element matching label, invoke action ...
    popup.Consume();

    return true;
}
```

**Key insight:** The Popup's `Routine()` method just loops checking `m_consumed`. It doesn't hold any lock while doing so. The main thread's `renderPopups()` holds `m_popupMutex` while rendering. So `respondToPopup` can safely:
1. Lock `m_popupMutex`
2. Modify inputs on the Popup
3. Call the button action
4. Set `m_consumed = true`

After that, the Lua worker's `Routine()` loop sees `m_consumed == true`, exits, reads the inputs, and removes the popup from the map.

**What needs to be exposed on Orchestrator:**

```cpp
// Add to orchestrator.h:
std::map<std::string, Popup>& getPopups() { return m_popups; }
std::mutex* getPopupMutex() { return m_popupMutex.get(); }
```

**Changes:**
- New: `Frasy/src/utils/mcp/unified_popup_handler.h/.cpp`
- Modified: `Frasy/src/utils/lua/orchestrator/orchestrator.h` — expose `getPopups()` and `getPopupMutex()`

---

### Task 6: SSE Progress Notifications

**Objective:** When a client opens a `GET /mcp` SSE stream, push real-time progress events and popup notifications proactively (server-to-client).

**Implementation:**

The `GET /mcp` handler keeps the connection open as an SSE stream. The main thread's `onUpdate()` detects state changes and pushes events to all connected SSE clients.

```cpp
// SSE event types
// Server → Client notifications (JSON-RPC notifications, no id)

// Progress
{"jsonrpc":"2.0","method":"notifications/progress","params":{"type":"test_start","uut":1,"sequence":"Power On","test":"Check Voltage"}}

// Popup appeared
{"jsonrpc":"2.0","method":"notifications/popup","params":{"id":"UUT1_Confirm Fixture","uut":1,"name":"Confirm Fixture","texts":[...],"inputs":[...],"buttons":[...]}}

// Popup consumed
{"jsonrpc":"2.0","method":"notifications/popup_consumed","params":{"id":"UUT1_Confirm Fixture","responded_by":"gui"}}

// Run state change
{"jsonrpc":"2.0","method":"notifications/state_change","params":{"state":"running","initiated_by":"mcp"}}
```

**cpp-httplib SSE support:**

cpp-httplib doesn't have built-in SSE, but it supports chunked responses which we can use:

```cpp
m_httpServer.Get("/mcp", [this](const httplib::Request& req, httplib::Response& res) {
    res.set_header("Content-Type", "text/event-stream");
    res.set_header("Cache-Control", "no-cache");
    res.set_header("Connection", "keep-alive");

    res.set_chunked_content_provider("text/event-stream",
        [this](size_t offset, httplib::DataSink& sink) {
            // Block here, writing SSE events as they come
            // Use a condition variable to wake when new events arrive
            // Return false to close the stream
        });
});
```

**Event queue per SSE connection:**

Each connected SSE client gets its own event queue. The main thread pushes events to all queues. The SSE handler thread drains its queue and writes to the stream.

**Changes:**
- Part of `Frasy/src/layers/mcp_http_server_layer.cpp`
- May need a `SseConnectionManager` helper class

---

### Task 7: Run Ownership and Conflict Prevention

**Objective:** Track who started the current test run and prevent conflicts.

**Implementation:**

```cpp
enum class RunOwner { None, Gui, Mcp };

// In McpHttpServerLayer (or shared via Interpreter)
struct RunState {
    std::atomic<RunOwner> owner = RunOwner::None;
    std::string initiator;  // operator name or MCP session ID
};
```

**Rules:**
1. `m_orchestrator.isRunning()` is the single source of truth for "is a run active."
2. Before starting a run (GUI or MCP), check `isRunning()`. If already running, reject.
3. Set `RunState::owner` when a run starts. Reset to `None` when the run completes (in the `onDone` callback).
4. `get_status` response includes `"initiated_by": "gui"` or `"initiated_by": "mcp"`.
5. GUI: if MCP started the run, show progress normally but indicate "Remote run in progress" instead of showing the operator's run button as active.
6. MCP: if GUI started the run, `run_tests` returns `{"error": "Tests already running (initiated by GUI operator)"}`.
7. Both GUI and MCP can abort any run via the abort button / `abort` tool.

**Changes:**
- Add `RunState` to `McpHttpServerLayer` (or a shared location)
- Modify GUI `doTests()` to set owner
- Modify MCP `handleRunTests` to set owner
- Add `abort` tool to MCP
- GUI: show indicator when MCP is controlling a run

---

### Task 8: Product State Sharing

**Objective:** The MCP layer needs to know what product is loaded and what products are available, using the same state as the GUI.

**Implementation:**

The `ProductProvider::listProducts()` already provides the canonical product list. The active product is tracked by the application's `MyMainApplicationLayer::m_activeProduct`.

**Approach:** The `McpHttpServerLayer` receives a callback or reference to query the active product:

```cpp
using ActiveProductGetter = std::function<std::string()>;
using ProductLoader = std::function<bool(const std::string& productName)>;

// Set during layer binding:
m_getActiveProduct = [&layer]() { return layer.getActiveProduct(); };
m_loadProduct = [&layer](const std::string& name) { layer.loadProduct(name); /* main-thread only */ };
```

For `load_product` (MCP tool): enqueue via command queue → main thread calls the application's `loadProduct()` → GUI updates product dropdown, device viewer reinitializes, etc.

For `list_products`: call `m_provider.listProducts()` directly (read-only).

**Changes:**
- Add `getActiveProduct()` getter to application layer (or expose via a shared interface)
- MCP `load_product` handler uses command queue

---

### Task 9: MCP Client/Relay Mode

**Objective:** `frasy.exe --mcp-client --address <addr> --port <port>` launches a lightweight process that bridges stdio ↔ HTTP, allowing AI agents to connect to a running GUI instance.

**Implementation:**

```cpp
// Frasy/src/utils/mcp/mcp_relay.h
namespace Frasy::Mcp {

class McpRelay {
public:
    McpRelay(const std::string& address, int port);

    /// Run the relay. Reads JSON-RPC from stdin, POSTs to HTTP,
    /// returns responses to stdout. Blocks until stdin closes.
    int run();

private:
    std::string m_url;  // e.g., "http://192.168.0.105:69/mcp"
    httplib::Client m_client;
};

}
```

**Relay logic:**

```
┌──────────┐ stdin  ┌───────────┐ POST /mcp  ┌──────────────┐
│ AI Agent │ ─────→ │ McpRelay  │ ─────────→ │ Primary GUI  │
│ (Kiro)   │ ←───── │           │ ←───────── │ HTTP Server  │
└──────────┘ stdout └───────────┘  response   └──────────────┘
```

1. Read line from stdin (JSON-RPC message).
2. HTTP POST to `http://{address}:{port}/mcp` with the message as body.
3. Read HTTP response body (JSON-RPC response).
4. Write response to stdout.
5. Optionally: maintain a background SSE connection (`GET /mcp`) to receive server notifications and forward them to stdout.

**SSE relay (for proactive notifications):**
- Spawn a background thread that connects `GET /mcp` as SSE.
- When SSE events arrive (progress, popups), write them to stdout as JSON-RPC notifications.
- This allows the AI agent to receive progress updates without polling.

**Note on process weight:** The `--mcp-client` process does NOT create an Application, does NOT initialize OpenGL, and does NOT register a ProductProvider. It's a minimal relay that only needs `cpp-httplib` and basic I/O. It exits the entry point before `CreateApplication()` is called.

**Changes:**
- New: `Frasy/src/utils/mcp/mcp_relay.h/.cpp`
- Modified: `Brigerad/src/Brigerad/Core/EntryPoint.h` — early exit for `--mcp-client` before app creation

---

### Task 10: Orchestrator Popup Access Refactor

**Objective:** Expose the necessary popup internals so the unified popup handler can read popup state and inject responses.

**Current state:** `Popup` stores elements as `std::vector<std::unique_ptr<Element>>` with specific subtypes (`Text`, `Input`, `Button`). The MCP handler needs to:
1. Read text/input/button info from a Popup (for `get_pending_popup`).
2. Set input values (for `respond_to_popup`).
3. Trigger a button action + consume (for `respond_to_popup`).

**Implementation:**

Add accessor methods to `Popup`:

```cpp
// In popup.h
class Popup {
public:
    // ... existing ...

    // New accessors for MCP integration:
    const std::vector<std::unique_ptr<Element>>& getElements() const { return m_elements; }
    void setInput(std::size_t index, const std::string& value);
    bool clickButton(const std::string& label);  // Calls action + Consume if button.consume
};
```

```cpp
// In popup.cpp
void Popup::setInput(std::size_t index, const std::string& value)
{
    std::size_t inputIdx = 0;
    for (auto& elem : m_elements) {
        if (elem->kind == Element::Kind::Input) {
            if (inputIdx == index) {
                auto* input = static_cast<Input*>(elem.get());
                std::ranges::copy(value, input->buffer.begin());
                input->buffer[std::min(value.size(), input->buffer.size() - 1)] = '\0';
                if (input->onChange) input->onChange(value, input->index);
                m_inputs[input->index] = value;
                return;
            }
            inputIdx++;
        }
    }
}

bool Popup::clickButton(const std::string& label)
{
    for (auto& elem : m_elements) {
        if (elem->kind == Element::Kind::Button) {
            auto* btn = static_cast<Button*>(elem.get());
            if (btn->label == label) {
                std::shared_lock lock(m_luaMutex);
                if (btn->action.valid()) {
                    btn->action(m_inputs);
                }
                if (btn->consume) {
                    Consume();
                }
                return true;
            }
        }
    }
    return false;
}
```

**Changes:**
- Modified: `Frasy/src/utils/lua/popup.h` — add `getElements()`, `setInput()`, `clickButton()`
- Modified: `Frasy/src/utils/lua/popup.cpp` — implement new methods
- Modified: `Frasy/src/utils/lua/orchestrator/orchestrator.h` — expose `getPopups()`, `getPopupMutex()`

---

### Task 11: Integration — Framework Level

**Objective:** Wire the MCP HTTP server layer into the Frasy framework's `MainApplicationLayer` so any application automatically gets MCP support.

**Implementation:**

In `MainApplicationLayer::onAttach()`:

```cpp
void MainApplicationLayer::onAttach()
{
    // ... existing initialization ...

    if (CliArgs::get().mcpPort > 0 || CliArgs::get().mcpPort == 0 /* auto-port */) {
        auto* provider = Interpreter::Get().getProductProvider();
        if (provider) {
            m_mcpLayer = std::make_unique<McpHttpServerLayer>(
                CliArgs::get().mcpPort,
                m_orchestrator,
                m_canOpen,
                *provider);
            m_mcpLayer->onAttach();
        }
    }
}
```

Or, better as a proper layer pushed to the layer stack:

```cpp
void MainApplicationLayer::onAttach()
{
    // ... existing ...

    if (CliArgs::get().mcpPort >= 0) {  // -1 = disabled, 0 = auto, >0 = specific port
        auto* provider = Interpreter::Get().getProductProvider();
        if (provider) {
            Brigerad::Application::Get().pushLayer(
                new McpHttpServerLayer(CliArgs::get().mcpPort, m_orchestrator, m_canOpen, *provider));
        }
    }
}
```

**The GUI continues to own m_orchestrator and m_canOpen.** The MCP layer only has references.

**Changes:**
- Modified: `Frasy/src/layers/main_application_layer.cpp` — conditionally push MCP layer
- Modified: `Frasy/src/layers/main_application_layer.h` — optional pointer to MCP layer (for RunState access)

---

### Task 12: Integration — Application Level

**Objective:** The Kinova frasy-application's `MyMainApplicationLayer` needs minor adjustments to expose active product info and handle MCP-initiated runs gracefully.

**Implementation:**

1. **Active product getter:** Already tracked in `m_activeProduct`. Expose via a virtual method or callback.
2. **Run indicator:** When `RunState::owner == Mcp`, show "Test run in progress (remote)" in the control room instead of the normal run button.
3. **Product change notification:** When MCP calls `load_product`, the GUI's product dropdown and loaded state update automatically (since `loadProduct()` runs on the main thread).

**Changes:**
- Minor modifications to `frasy-application/src/layers/my_main_application_layer.cpp`

---

### Task 13: Backward Compatibility

**Objective:** Ensure all existing modes continue working.

| Mode | Status |
|------|--------|
| `frasy.exe` | Unchanged — no MCP server |
| `frasy.exe --mcp-server` (standalone) | Unchanged — standalone stdio MCP with own orchestrator (deprecated) |
| `frasy.exe --headless ...` | Unchanged |
| `frasy.exe --mcp-port 8080` | **New** — GUI + HTTP MCP server |
| `frasy.exe --mcp-client --port 8080` | **New** — relay mode (stdio ↔ HTTP to localhost) |
| `frasy.exe --mcp-client --address x --port y` | **New** — remote relay mode |

The standalone `--mcp-server` mode is deprecated but fully preserved for backward compatibility. It can be removed in a future major version once all users migrate to the HTTP-based approach.

**Mutual exclusivity:**
- `--mcp-port` is incompatible with `--mcp-server`, `--mcp-client`, `--headless`.
- `--mcp-client` is incompatible with `--mcp-server`, `--mcp-port`, `--headless`.
- `--mcp-server` is incompatible with `--mcp-client`, `--mcp-port`, `--headless`.

---

## Implementation Order

```
Phase 1: Foundation
├── Task 1:  Add cpp-httplib dependency
├── Task 2:  CLI flags (--mcp-port, --address, --port)
└── Task 10: Popup access refactor (expose getPopups, add setInput/clickButton)

Phase 2: Core Server
├── Task 3:  McpHttpServerLayer (HTTP server + JSON-RPC dispatch)
├── Task 4:  Command queue for thread-safe mutating ops
└── Task 8:  Product state sharing

Phase 3: Dual-Path Interaction
├── Task 5:  Unified popup handler (dual-path, first-responder-wins)
├── Task 6:  SSE progress notifications
└── Task 7:  Run ownership and conflict prevention

Phase 4: Client Mode
└── Task 9:  MCP client relay (--mcp-client, stdio ↔ HTTP bridge)

Phase 5: Integration
├── Task 11: Framework-level wiring
├── Task 12: Application-level adjustments
└── Task 13: Backward compatibility testing
```

---

## Detailed Interaction Flows

### Flow A: MCP Agent Starts a Test Run (Operator Watches)

```
Agent                        HTTP Server          Main Thread (GUI)         Lua Workers
  |                              |                      |                       |
  |--POST run_tests------------>|                      |                       |
  |                              |--enqueue command---->|                       |
  |                              |                      |--loadProduct()        |
  |                              |                      |--runSolution()------->|
  |                              |<--result: started----|                       |
  |<-response: {started:true}----|                      |                       |
  |                              |                      |                       |
  |  (SSE stream)               |                      |                       |
  |<--notification: running------|<--state change-------|                       |
  |<--notification: test_start---|<--progress event-----|                       |
  |                              |                      |--renderPopups()       |
  |                              |                      |   (operator sees      |
  |                              |                      |    progress in GUI)   |
```

### Flow B: Popup — Agent Responds Before Operator

```
Lua Worker                   Main Thread              HTTP Server           Agent
  |                              |                      |                    |
  |--Show(popup)---------------->|                      |                    |
  |  (blocks in Routine())       |                      |                    |
  |                              |--renderPopups()      |                    |
  |                              |  (ImGui popup shown) |                    |
  |                              |--sync() detects new->|--SSE: popup------->|
  |                              |                      |                    |
  |                              |                      |<-respond_to_popup--|
  |                              |                      |--setInput()+       |
  |                              |                      |  clickButton()     |
  |                              |                      |  Consume()         |
  |  (Routine() sees consumed)   |                      |                    |
  |<--(unblocked)                |                      |                    |
  |                              |--next renderPopups() |                    |
  |                              |  (popup gone from    |                    |
  |                              |   m_popups → ImGui   |                    |
  |                              |   popup disappears)  |                    |
```

### Flow C: Popup — Operator Responds Before Agent

```
Lua Worker                   Main Thread              HTTP Server           Agent
  |                              |                      |                    |
  |--Show(popup)---------------->|                      |                    |
  |  (blocks in Routine())       |                      |                    |
  |                              |--renderPopups()      |                    |
  |                              |  (operator clicks    |                    |
  |                              |   button → Consume())|                    |
  |  (Routine() sees consumed)   |                      |                    |
  |<--(unblocked, reads inputs)  |                      |                    |
  |  (popup removed from map)    |                      |                    |
  |                              |--sync() detects      |                    |
  |                              |  popup gone -------->|--SSE: consumed---->|
  |                              |                      |                    |
  |                              |                      |<-respond_to_popup--|
  |                              |                      |  (returns error:   |
  |                              |                      |   popup not found) |
```

### Flow D: Remote Agent Connects via Relay (`--mcp-client`)

```
Agent (Kiro)           McpRelay Process          Primary GUI Instance
  |                        |                          |
  |--stdin: initialize---->|                          |
  |                        |--POST /mcp: initialize-->|
  |                        |<-response: result--------|
  |<-stdout: result--------|                          |
  |                        |                          |
  |--stdin: tools/call---->|                          |
  |                        |--POST /mcp: tools/call-->|
  |                        |<-response: result--------|
  |<-stdout: result--------|                          |
  |                        |                          |
  |                        |--GET /mcp (SSE)--------->|
  |                        |<-SSE: progress events----|
  |<-stdout: notifications-|                          |
```

---

## Security Considerations

1. **Origin validation:** HTTP server validates `Origin` header. Accept requests only from `localhost` or explicitly configured origins.
2. **Localhost binding by default:** Server binds to `127.0.0.1` by default (not `0.0.0.0`) unless explicitly configured otherwise.
3. **Session management:** Use `Mcp-Session-Id` header per the spec. Reject requests without valid session after initialization.
4. **No authentication initially:** For local development use, no auth. Document that exposing to network requires proper auth (future enhancement).

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Thread race on orchestrator state | Medium | High | Command queue for mutations; careful classification of read-only ops |
| Popup dual-path race condition | Medium | Medium | Use `m_consumed` atomic + popup mutex; first-responder wins naturally |
| cpp-httplib blocking thread exhaustion | Low | Medium | Set reasonable thread pool size; tool calls are fast |
| Port conflict on startup | Low | Low | Support port 0 (auto); clear error message if port in use |
| GUI and MCP disagree on state | Low | Low | Single orchestrator instance = single source of truth |
| Button action requires Lua state | Medium | Medium | Button actions run on the Lua worker thread after Routine() unblocks — need to verify thread safety |

---

## Open Questions (Resolved)

| Question | Decision |
|----------|----------|
| Shared orchestrator? | ✅ Yes — same instance for GUI and MCP |
| Dual-path popups? | ✅ Yes — from day one |
| Transport? | ✅ HTTP (Streamable HTTP spec) hosted by GUI process |
| Remote connection? | ✅ Via relay process (`--mcp-client --address x --port y`) |
| Backward compat? | ✅ Standalone `--mcp-server` preserved |
| Should the GUI display a status bar showing MCP connection count / active session? | Yes, next to the Connected tab |

## Remaining Open Questions

1. **Port default:** Should `--mcp-port` have a default (e.g., always-on at port 8042)? Or require explicit opt-in?
   - Recommendation: Require explicit opt-in (`--mcp-port <N>`) for now. Consider a config.json option to auto-start.

2. **Multiple concurrent MCP clients:** The HTTP server naturally supports this. Should we limit to one session?
   - Recommendation: Allow multiple read-only clients, but only one can have an active `run_tests` session.

3. **Button action thread safety:** The `Popup::Button::action` is a `sol::unsafe_function` that captures the Lua state. Calling it from the HTTP thread (via `respondToPopup`) would be unsafe. Need to ensure button actions are deferred to the Lua worker thread.
   - Resolution: `respondToPopup` sets inputs and marks consumed, but does NOT call the button action directly. The Lua worker's `Routine()` exit path already handles calling the action. We just need to store WHICH button was clicked.

4. **Should the GUI display a status bar showing MCP connection count / active session?**
   - Recommendation: Yes, a small indicator in the menu bar or status area. Low priority.

---

## Test Strategy

- **Unit tests:** JSON-RPC dispatch, command queue, popup tracking/sync logic
- **Integration test:** Start Frasy with `--mcp-port`, send HTTP requests to `/mcp`, verify tool responses
- **Relay test:** Start Frasy with `--mcp-port`, start relay process (`--mcp-client --port`), pipe JSON-RPC via stdin, verify end-to-end
- **Popup dual-path test:** Start run, trigger popup, respond via HTTP, verify GUI popup disappears
- **Conflict test:** Start run via MCP, try GUI run (expect rejection). Vice versa.
- **SSE test:** Connect SSE stream, start run, verify progress notifications arrive
- **Shutdown test:** Close GUI with active SSE clients — verify clean disconnect
- **Backward compat:** Verify `--mcp-server` (standalone) still works identically
