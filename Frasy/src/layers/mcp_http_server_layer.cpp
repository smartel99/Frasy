/**
 * @file    mcp_http_server_layer.cpp
 * @author  Frasy
 * @date    2026-08-17
 * @brief   MCP HTTP Server Layer implementation.
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "mcp_http_server_layer.h"

#include "utils/misc/deserializer.h"

#include <Brigerad/Core/Log.h>

#include <httplib.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>

namespace Frasy {

namespace {
constexpr auto s_protocolVersion = "2024-11-05";
constexpr auto s_mcpEndpoint     = "/mcp";

std::string generateSessionId()
{
    static std::mt19937_64            rng(std::random_device {}());
    static std::uniform_int_distribution<uint64_t> dist;
    auto a = dist(rng);
    auto b = dist(rng);
    return std::format("{:016x}{:016x}", a, b);
}
}    // namespace

// --- Construction / Destruction ---

McpHttpServerLayer::McpHttpServerLayer(int                        port,
                                       Lua::Orchestrator&         orchestrator,
                                       CanOpen::CanOpen&          canOpen,
                                       Headless::ProductProvider& provider)
: m_requestedPort(port), m_orchestrator(orchestrator), m_canOpen(canOpen), m_provider(provider)
{
}

McpHttpServerLayer::~McpHttpServerLayer()
{
    if (m_httpServer) { m_httpServer->stop(); }
    if (m_serverThread.joinable()) { m_serverThread.join(); }
}

// --- Layer lifecycle ---

void McpHttpServerLayer::onAttach()
{
    registerTools();

    m_httpServer = std::make_unique<httplib::Server>();

    // POST /mcp — JSON-RPC tool calls
    m_httpServer->Post(s_mcpEndpoint, [this](const httplib::Request& req, httplib::Response& res) {
        handlePost(req, res);
    });

    // GET /mcp — SSE stream (placeholder for now)
    m_httpServer->Get(s_mcpEndpoint, [this](const httplib::Request& req, httplib::Response& res) {
        handleGet(req, res);
    });

    // DELETE /mcp — session termination
    m_httpServer->Delete(s_mcpEndpoint, [this](const httplib::Request& req, httplib::Response& res) {
        handleDelete(req, res);
    });

    // Start the server on a background thread
    m_serverThread = std::jthread([this](std::stop_token) {
        int port = m_requestedPort;
        if (port == 0) {
            // Auto-select port
            port = m_httpServer->bind_to_any_port("127.0.0.1");
            if (port < 0) {
                BR_LOG_ERROR(s_tag, "Failed to bind to any port");
                return;
            }
            m_listeningPort = port;
            BR_LOG_INFO(s_tag, "MCP HTTP server bound to auto-selected port {}", port);
            m_httpServer->listen_after_bind();
        }
        else {
            m_listeningPort = port;
            BR_LOG_INFO(s_tag, "MCP HTTP server starting on port {}", port);
            if (!m_httpServer->listen("127.0.0.1", port)) {
                BR_LOG_ERROR(s_tag, "Failed to start HTTP server on port {}", port);
            }
        }
        BR_LOG_INFO(s_tag, "MCP HTTP server stopped");
    });

    // Give the server a moment to bind
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    BR_LOG_INFO(s_tag, "MCP HTTP server layer attached (port {})", m_listeningPort);
}

void McpHttpServerLayer::onDetach()
{
    // Close all SSE connections
    {
        std::lock_guard lock(m_sseMutex);
        for (auto& conn : m_sseConnections) {
            conn->closed = true;
            conn->cv.notify_all();
        }
        m_sseConnections.clear();
    }

    if (m_httpServer) {
        m_httpServer->stop();
    }
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
    BR_LOG_INFO(s_tag, "MCP HTTP server layer detached");
}

void McpHttpServerLayer::onUpdate(Brigerad::Timestep /*ts*/)
{
    // Drain command queue on main thread
    {
        std::lock_guard lock(m_commandMutex);
        while (!m_commandQueue.empty()) {
            auto cmd = std::move(m_commandQueue.front());
            m_commandQueue.pop();
            try {
                cmd.result.set_value(cmd.execute());
            }
            catch (const std::exception& e) {
                nlohmann::json err;
                err["error"] = e.what();
                cmd.result.set_value(err);
            }
        }
    }

    // Sync popup state with orchestrator
    m_popupHandler.sync(m_orchestrator);

    // Push SSE notifications for new/consumed popups
    for (const auto& popup : m_popupHandler.consumeNewPopups()) {
        nlohmann::json event;
        event["jsonrpc"]         = "2.0";
        event["method"]          = "notifications/popup";
        event["params"]["id"]    = popup.id;
        event["params"]["texts"] = popup.texts;

        nlohmann::json inputs = nlohmann::json::array();
        for (std::size_t i = 0; i < popup.inputTitles.size(); ++i) {
            inputs.push_back({{"index", i}, {"title", popup.inputTitles[i]}});
        }
        event["params"]["inputs"]  = inputs;
        event["params"]["buttons"] = popup.buttons;
        pushSseEvent(event);
    }

    for (const auto& id : m_popupHandler.consumeRemovedPopups()) {
        nlohmann::json event;
        event["jsonrpc"]       = "2.0";
        event["method"]        = "notifications/popup_consumed";
        event["params"]["id"]  = id;
        pushSseEvent(event);
    }

    // Clean up closed SSE connections periodically
    cleanClosedSseConnections();
}

// --- SSE helpers ---

void McpHttpServerLayer::pushSseEvent(const nlohmann::json& event)
{
    std::string serialized = event.dump();
    std::lock_guard lock(m_sseMutex);
    for (auto& conn : m_sseConnections) {
        if (!conn->closed) {
            std::lock_guard connLock(conn->mutex);
            conn->events.push(serialized);
            conn->cv.notify_one();
        }
    }
}

void McpHttpServerLayer::cleanClosedSseConnections()
{
    std::lock_guard lock(m_sseMutex);
    std::erase_if(m_sseConnections, [](const auto& conn) { return conn->closed.load(); });
}

// --- Command queue ---

nlohmann::json McpHttpServerLayer::enqueueAndWait(std::function<nlohmann::json()> fn)
{
    Command cmd;
    cmd.execute = std::move(fn);
    auto future = cmd.result.get_future();
    {
        std::lock_guard lock(m_commandMutex);
        m_commandQueue.push(std::move(cmd));
    }
    return future.get();
}

// --- HTTP handlers ---

void McpHttpServerLayer::handlePost(const httplib::Request& req, httplib::Response& res)
{
    // Validate Origin header for DNS rebinding protection
    if (req.has_header("Origin")) {
        auto origin = req.get_header_value("Origin");
        if (origin.find("localhost") == std::string::npos &&
            origin.find("127.0.0.1") == std::string::npos) {
            res.status = 403;
            res.set_content("Forbidden: invalid origin", "text/plain");
            return;
        }
    }

    // Parse JSON-RPC body
    nlohmann::json message;
    try {
        message = nlohmann::json::parse(req.body);
    }
    catch (const nlohmann::json::exception&) {
        auto error = makeJsonRpcError(nullptr, -32700, "Parse error");
        res.set_content(error.dump(), "application/json");
        return;
    }

    // Validate session (after initialize)
    if (m_initialized && !m_sessionId.empty()) {
        auto clientSession = req.get_header_value("Mcp-Session-Id");
        if (clientSession != m_sessionId) {
            // Check if this is an initialize request (allowed without session)
            if (!message.contains("method") || message["method"] != "initialize") {
                res.status = 400;
                res.set_content("Bad Request: invalid or missing Mcp-Session-Id", "text/plain");
                return;
            }
        }
    }

    // Dispatch
    auto response = dispatch(message);

    // Set session header on initialize response
    if (message.contains("method") && message["method"] == "initialize" && !m_sessionId.empty()) {
        res.set_header("Mcp-Session-Id", m_sessionId);
    }

    res.set_content(response.dump(), "application/json");
}

void McpHttpServerLayer::handleGet(const httplib::Request& /*req*/, httplib::Response& res)
{
    // SSE stream — server-to-client notifications
    auto conn = std::make_shared<SseConnection>();
    {
        std::lock_guard lock(m_sseMutex);
        m_sseConnections.push_back(conn);
    }

    res.set_header("Cache-Control", "no-cache");
    res.set_header("Connection", "keep-alive");

    res.set_chunked_content_provider("text/event-stream",
        [conn](size_t /*offset*/, httplib::DataSink& sink) {
            // Wait for events or closure
            std::unique_lock lock(conn->mutex);
            conn->cv.wait(lock, [&conn] {
                return !conn->events.empty() || conn->closed;
            });

            if (conn->closed) { return false; }

            // Drain event queue
            while (!conn->events.empty()) {
                auto event = std::move(conn->events.front());
                conn->events.pop();
                std::string sseData = "data: " + event + "\n\n";
                sink.write(sseData.c_str(), sseData.size());
            }
            return true;
        },
        [conn](bool /*success*/) {
            // On close
            conn->closed = true;
            conn->cv.notify_all();
        });
}

void McpHttpServerLayer::handleDelete(const httplib::Request& req, httplib::Response& res)
{
    auto clientSession = req.get_header_value("Mcp-Session-Id");
    if (clientSession == m_sessionId) {
        m_initialized = false;
        m_sessionId.clear();
        res.status = 200;
        res.set_content("Session terminated", "text/plain");
        BR_LOG_INFO(s_tag, "Session terminated by client");
    }
    else {
        res.status = 404;
        res.set_content("Session not found", "text/plain");
    }
}

// --- JSON-RPC dispatch ---

nlohmann::json McpHttpServerLayer::dispatch(const nlohmann::json& message)
{
    // Validate basic JSON-RPC structure
    if (!message.contains("jsonrpc") || message["jsonrpc"] != "2.0") {
        return makeJsonRpcError(nullptr, -32600, "Invalid Request: not JSON-RPC 2.0");
    }

    bool isNotification = !message.contains("id");
    auto id             = isNotification ? nlohmann::json(nullptr) : message["id"];

    if (!message.contains("method")) {
        if (!isNotification) { return makeJsonRpcError(id, -32600, "Invalid Request: missing method"); }
        return {};
    }

    std::string method = message["method"];
    auto        params = message.value("params", nlohmann::json::object());

    // Handle notifications
    if (method == "notifications/initialized") {
        BR_LOG_INFO(s_tag, "Client initialized notification received");
        return {};
    }
    if (method == "notifications/cancelled") {
        BR_LOG_WARN(s_tag, "Client cancelled request");
        return {};
    }
    if (isNotification) { return {}; }

    // Handle requests
    if (method == "initialize") {
        auto result = handleInitialize(params);
        return makeJsonRpcResponse(id, result);
    }
    if (method == "tools/list") {
        auto result = handleToolsList();
        return makeJsonRpcResponse(id, result);
    }
    if (method == "tools/call") {
        auto result = handleToolsCall(params);
        return makeJsonRpcResponse(id, result);
    }
    if (method == "ping") {
        return makeJsonRpcResponse(id, nlohmann::json::object());
    }

    return makeJsonRpcError(id, -32601, "Method not found: " + method);
}

nlohmann::json McpHttpServerLayer::handleInitialize(const nlohmann::json& params)
{
    BR_LOG_INFO(s_tag, "Client connecting: {}",
                params.value("clientInfo", nlohmann::json::object()).value("name", "unknown"));

    m_initialized = true;
    m_sessionId   = generateSessionId();

    nlohmann::json result;
    result["protocolVersion"]       = s_protocolVersion;
    result["capabilities"]["tools"] = nlohmann::json::object();
    result["serverInfo"]["name"]    = "frasy";
    result["serverInfo"]["version"] = "1.0.0";
    return result;
}

nlohmann::json McpHttpServerLayer::handleToolsList()
{
    nlohmann::json tools = nlohmann::json::array();
    for (const auto& [name, tool] : m_tools) {
        tools.push_back({
          {"name", tool.name},
          {"description", tool.description},
          {"inputSchema", tool.inputSchema},
        });
    }
    nlohmann::json result;
    result["tools"] = tools;
    return result;
}

nlohmann::json McpHttpServerLayer::handleToolsCall(const nlohmann::json& params)
{
    std::string name = params.value("name", "");
    auto        args = params.value("arguments", nlohmann::json::object());

    auto it = m_tools.find(name);
    if (it == m_tools.end()) {
        return makeToolResult("Unknown tool: " + name, true);
    }

    try {
        return it->second.handler(args);
    }
    catch (const std::exception& e) {
        return makeToolResult(std::string("Tool error: ") + e.what(), true);
    }
}

// --- Helpers ---

nlohmann::json McpHttpServerLayer::makeToolResult(const std::string& text, bool isError)
{
    nlohmann::json result;
    result["content"] = nlohmann::json::array({{{"type", "text"}, {"text", text}}});
    result["isError"] = isError;
    return result;
}

nlohmann::json McpHttpServerLayer::makeJsonRpcResponse(const nlohmann::json& id, const nlohmann::json& result)
{
    nlohmann::json response;
    response["jsonrpc"] = "2.0";
    response["id"]      = id;
    response["result"]  = result;
    return response;
}

nlohmann::json McpHttpServerLayer::makeJsonRpcError(const nlohmann::json& id, int code, const std::string& message)
{
    nlohmann::json response;
    response["jsonrpc"]          = "2.0";
    response["id"]               = id;
    response["error"]["code"]    = code;
    response["error"]["message"] = message;
    return response;
}

std::vector<Headless::ProductInfo> McpHttpServerLayer::discoverProducts()
{
    auto providerProducts = m_provider.listProducts();
    if (!providerProducts.empty()) { return providerProducts; }

    namespace fs = std::filesystem;
    std::vector<Headless::ProductInfo> products;
    try {
        for (const auto& entry : fs::recursive_directory_iterator("lua/user")) {
            if (!entry.is_directory()) { continue; }
            auto environment = fs::directory_entry(entry.path() / "environment.lua");
            if (!environment.exists()) { continue; }
            auto envPath = environment.path();
            envPath.replace_extension();
            products.emplace_back(Headless::ProductInfo {
              .name            = entry.path().filename().string(),
              .environmentPath = envPath.string(),
              .testPath        = entry.path().string(),
            });
        }
    }
    catch (const fs::filesystem_error& e) {
        BR_LOG_ERROR(s_tag, "Error scanning for products: {}", e.what());
    }
    return products;
}

// --- Tool registration ---

void McpHttpServerLayer::registerTools()
{
    using json = nlohmann::json;

    m_tools["list_products"] = {
      "list_products",
      "List available test products",
      json({{"type", "object"}, {"properties", json::object()}}),
      [this](const json& args) { return handleListProducts(args); },
    };

    m_tools["load_product"] = {
      "load_product",
      "Load a test product into the orchestrator",
      json({{"type", "object"},
            {"properties", {{"product", {{"type", "string"}, {"description", "Product name to load"}}}}},
            {"required", json::array({"product"})}}),
      [this](const json& args) { return handleLoadProduct(args); },
    };

    m_tools["run_tests"] = {
      "run_tests",
      "Start a test run. Returns immediately; poll get_status for progress.",
      json({{"type", "object"},
            {"properties",
             {{"product", {{"type", "string"}, {"description", "Product name to test"}}},
              {"operator", {{"type", "string"}, {"description", "Operator name"}}},
              {"serials",
               {{"type", "array"}, {"items", {{"type", "string"}}}, {"description", "Serial numbers, one per UUT"}}},
              {"skip_verification",
               {{"type", "boolean"}, {"description", "Skip hash verification (default false)"}}}}},
            {"required", json::array({"product", "operator", "serials"})}}),
      [this](const json& args) { return handleRunTests(args); },
    };

    m_tools["get_status"] = {
      "get_status",
      "Get the current test execution status and per-UUT states",
      json({{"type", "object"}, {"properties", json::object()}}),
      [this](const json& args) { return handleGetStatus(args); },
    };

    m_tools["get_results"] = {
      "get_results",
      "Get the test results summary after execution completes",
      json({{"type", "object"}, {"properties", json::object()}}),
      [this](const json& args) { return handleGetResults(args); },
    };

    m_tools["get_pending_popup"] = {
      "get_pending_popup",
      "Get the next popup waiting for interaction. Returns null if none pending.",
      json({{"type", "object"}, {"properties", json::object()}}),
      [this](const json& args) { return handleGetPendingPopup(args); },
    };

    m_tools["respond_to_popup"] = {
      "respond_to_popup",
      "Respond to a pending popup by providing inputs and pressing a button",
      json({{"type", "object"},
            {"properties",
             {{"id", {{"type", "string"}, {"description", "Popup ID from get_pending_popup"}}},
              {"inputs",
               {{"type", "object"}, {"description", "Input values keyed by index (e.g. {\"1\": \"value\"})"}}},
              {"button", {{"type", "string"}, {"description", "Button label to press"}}}}},
            {"required", json::array({"id", "button"})}}),
      [this](const json& args) { return handleRespondToPopup(args); },
    };

    m_tools["abort"] = {
      "abort",
      "Abort the current test run",
      json({{"type", "object"}, {"properties", json::object()}}),
      [this](const json& args) { return handleAbort(args); },
    };

    m_tools["list_nodes"] = {
      "list_nodes",
      "Get a list of CANOpen nodes in the network",
      json({{"type", "object"}, {"properties", json::object()}}),
      [this](const json& args) { return handleListNodes(args); },
    };

    m_tools["list_devices"] = {
      "list_devices",
      "Get a list of connected COM port devices",
      json({{"type", "object"}, {"properties", json::object()}}),
      [this](const json& args) { return handleListDevices(args); },
    };

    m_tools["upload_sdo"] = {
      "upload_sdo",
      "Upload (read) a value from a CANOpen SDO",
      json({{"type", "object"},
            {"properties",
             {{"nodeId", {{"type", "number"}, {"description", "Node ID"}}},
              {"index", {{"type", "number"}, {"description", "SDO index"}}},
              {"sub-index", {{"type", "number"}, {"description", "SDO sub-index"}}},
              {"type",
               {{"type", "string"}, {"description", "CANOpen type (e.g. unsigned8, signed32, string)"}}}}},
            {"required", json::array({"nodeId", "index", "sub-index", "type"})}}),
      [this](const json& args) { return handleUploadSdo(args); },
    };
}

// --- Tool handlers ---

nlohmann::json McpHttpServerLayer::handleListProducts(const nlohmann::json& /*args*/)
{
    auto           products    = discoverProducts();
    nlohmann::json productList = nlohmann::json::array();
    for (const auto& p : products) {
        productList.push_back({{"name", p.name}});
    }
    nlohmann::json response;
    response["products"] = productList;
    return makeToolResult(response.dump());
}

nlohmann::json McpHttpServerLayer::handleLoadProduct(const nlohmann::json& args)
{
    std::string product = args.value("product", "");
    if (product.empty()) { return makeToolResult(R"({"error":"product is required"})", true); }

    // Mutating: enqueue to main thread
    return enqueueAndWait([this, product]() -> nlohmann::json {
        if (m_running) { return makeToolResult(R"({"error":"Tests are already running"})", true); }

        // Use the application's loadProduct callback if available
        if (m_loadProduct) {
            if (!m_loadProduct(product)) {
                return makeToolResult(
                  nlohmann::json({{"error", "Failed to load product"}, {"product", product}}).dump(), true);
            }
            m_activeProduct = product;
            return makeToolResult(R"({"loaded":true})");
        }

        // Fallback: direct setup via ProductProvider
        auto products = discoverProducts();
        auto it = std::ranges::find_if(products, [&](const Headless::ProductInfo& p) { return p.name == product; });
        if (it == products.end()) {
            nlohmann::json available = nlohmann::json::array();
            for (const auto& p : products) { available.push_back(p.name); }
            return makeToolResult(
              nlohmann::json({{"error", "Product not found"}, {"available", available}}).dump(), true);
        }

        if (!m_provider.setup(m_orchestrator, m_canOpen, it->name, it->environmentPath, it->testPath)) {
            return makeToolResult(R"({"error":"ProductProvider::setup() failed"})", true);
        }

        m_activeProduct = it->name;
        return makeToolResult(R"({"loaded":true})");
    });
}

nlohmann::json McpHttpServerLayer::handleRunTests(const nlohmann::json& args)
{
    std::string product          = args.value("product", "");
    std::string operatorName     = args.value("operator", "");
    auto        serials          = args.value("serials", std::vector<std::string> {});
    bool        skipVerification = args.value("skip_verification", false);

    if (product.empty() || operatorName.empty() || serials.empty()) {
        return makeToolResult(R"({"error":"product, operator, and serials are required"})", true);
    }

    // Validate serials
    for (const auto& sn : serials) {
        if (!m_provider.validateSerialNumber(sn)) {
            return makeToolResult(
              nlohmann::json({{"error", std::format("Invalid serial number: {}", sn)}}).dump(), true);
        }
    }

    // Mutating: enqueue to main thread
    return enqueueAndWait([this, product, operatorName, serials, skipVerification]() -> nlohmann::json {
        if (m_running) {
            std::string owner = m_runOwner == RunOwner::Gui ? "GUI operator" : "MCP agent";
            return makeToolResult(
              nlohmann::json({{"error", std::format("Tests already running (initiated by {})", owner)}}).dump(), true);
        }

        auto products = discoverProducts();
        auto it = std::ranges::find_if(products, [&](const Headless::ProductInfo& p) { return p.name == product; });
        if (it == products.end()) {
            nlohmann::json available = nlohmann::json::array();
            for (const auto& p : products) { available.push_back(p.name); }
            return makeToolResult(
              nlohmann::json({{"error", "Product not found"}, {"available", available}}).dump(), true);
        }

        if (!m_provider.setup(m_orchestrator, m_canOpen, it->name, it->environmentPath, it->testPath)) {
            return makeToolResult(R"({"error":"ProductProvider::setup() failed"})", true);
        }

        // Validate UUT count
        const auto& map = m_orchestrator.getMap();
        if (serials.size() != map.uuts.size()) {
            return makeToolResult(
              nlohmann::json(
                {{"error", "Serial count mismatch"}, {"provided", serials.size()}, {"expected", map.uuts.size()}})
                .dump(),
              true);
        }

        // Build serials vector (index 0 = copy of index 1)
        m_serials.clear();
        m_serials.reserve(serials.size() + 1);
        m_serials.push_back(serials.front());
        for (const auto& sn : serials) { m_serials.push_back(sn); }

        m_activeProduct = it->name;
        m_running       = true;
        m_runOwner      = RunOwner::Mcp;

        m_orchestrator.runSolution(operatorName, m_serials, true, skipVerification, [this] {
            m_running  = false;
            m_runOwner = RunOwner::None;
            m_provider.onTestComplete(m_orchestrator);
        });

        return makeToolResult(R"({"started":true})");
    });
}

nlohmann::json McpHttpServerLayer::handleGetStatus(const nlohmann::json& /*args*/)
{
    nlohmann::json status;

    if (m_running) { status["state"] = "running"; }
    else if (m_activeProduct.empty()) { status["state"] = "idle"; }
    else {
        const auto& map     = m_orchestrator.getMap();
        bool        anyFail = false;
        bool        anyErr  = false;
        for (const auto& uut : map.uuts) {
            auto s = m_orchestrator.getUutState(uut);
            if (s == UutState::Failed) { anyFail = true; }
            if (s == UutState::Error) { anyErr = true; }
        }
        if (anyErr) { status["state"] = "error"; }
        else if (anyFail) { status["state"] = "failed"; }
        else { status["state"] = "passed"; }
    }

    // Run owner
    switch (m_runOwner.load()) {
        case RunOwner::Gui: status["initiated_by"] = "gui"; break;
        case RunOwner::Mcp: status["initiated_by"] = "mcp"; break;
        default: status["initiated_by"] = "none"; break;
    }

    // Per-UUT states
    const auto&    map  = m_orchestrator.getMap();
    nlohmann::json uuts = nlohmann::json::array();
    for (const auto& uut : map.uuts) {
        auto        state = m_orchestrator.getUutState(uut);
        std::string stateStr;
        switch (state) {
            case UutState::Disabled: stateStr = "disabled"; break;
            case UutState::Idle: stateStr = "idle"; break;
            case UutState::Waiting: stateStr = "waiting"; break;
            case UutState::Running: stateStr = "running"; break;
            case UutState::Passed: stateStr = "passed"; break;
            case UutState::Failed: stateStr = "failed"; break;
            case UutState::Error: stateStr = "error"; break;
        }
        nlohmann::json uutInfo;
        uutInfo["uut"]    = uut;
        uutInfo["serial"] = (uut < m_serials.size()) ? m_serials[uut] : "";
        uutInfo["state"]  = stateStr;
        uuts.push_back(uutInfo);
    }
    status["uuts"]    = uuts;
    status["product"] = m_getActiveProduct ? m_getActiveProduct() : m_activeProduct;

    return makeToolResult(status.dump());
}

nlohmann::json McpHttpServerLayer::handleGetResults(const nlohmann::json& /*args*/)
{
    if (m_running) { return makeToolResult("{\"error\":\"Tests are still running\"}", true); }
    if (m_activeProduct.empty()) {
        return makeToolResult("{\"error\":\"No test results available (no tests have been run)\"}", true);
    }

    const auto&    map = m_orchestrator.getMap();
    nlohmann::json results;
    results["product"]      = m_activeProduct;
    results["overall_pass"] = true;

    nlohmann::json uutResults = nlohmann::json::array();
    for (const auto& uut : map.uuts) {
        auto state = m_orchestrator.getUutState(uut);
        bool pass  = (state == UutState::Passed);
        if (!pass) { results["overall_pass"] = false; }

        nlohmann::json uutResult;
        uutResult["uut"]    = uut;
        uutResult["serial"] = (uut < m_serials.size()) ? m_serials[uut] : "";
        uutResult["pass"]   = pass;

        std::string stateStr;
        switch (state) {
            case UutState::Passed: stateStr = "passed"; break;
            case UutState::Failed: stateStr = "failed"; break;
            case UutState::Error: stateStr = "error"; break;
            default: stateStr = "unknown"; break;
        }
        uutResult["state"] = stateStr;

        std::string reportPath = std::format("logs/last/{}.json", uut);
        if (std::filesystem::exists(reportPath)) {
            uutResult["report_path"] = reportPath;
            try {
                std::ifstream ifs(reportPath);
                auto          report = nlohmann::json::parse(ifs);
                if (report.contains("info")) {
                    uutResult["duration"] =
                      report["info"].value("time", nlohmann::json::object()).value("elapsed", 0.0);
                }
                int totalTests  = 0;
                int passedTests = 0;
                if (report.contains("sequences")) {
                    for (auto& [seqName, seq] : report["sequences"].items()) {
                        if (seq.contains("tests")) {
                            for (auto& [testName, test] : seq["tests"].items()) {
                                totalTests++;
                                if (test.value("pass", false)) { passedTests++; }
                            }
                        }
                    }
                }
                uutResult["tests_passed"] = passedTests;
                uutResult["tests_total"]  = totalTests;
            }
            catch (...) {}
        }
        uutResults.push_back(uutResult);
    }
    results["uuts"] = uutResults;
    return makeToolResult(results.dump());
}

nlohmann::json McpHttpServerLayer::handleGetPendingPopup(const nlohmann::json& /*args*/)
{
    auto popup = m_popupHandler.getPendingPopup();
    if (popup.has_value()) { return makeToolResult(popup->dump()); }
    return makeToolResult(R"({"popup":null})");
}

nlohmann::json McpHttpServerLayer::handleRespondToPopup(const nlohmann::json& args)
{
    std::string id     = args.value("id", "");
    std::string button = args.value("button", "");

    if (id.empty() || button.empty()) {
        return makeToolResult(R"({"error":"id and button are required"})", true);
    }

    // Parse inputs
    std::map<std::size_t, std::string> inputValues;
    if (args.contains("inputs") && args["inputs"].is_object()) {
        for (auto& [key, value] : args["inputs"].items()) {
            try {
                inputValues[std::stoull(key)] = value.get<std::string>();
            }
            catch (...) {}
        }
    }

    if (m_popupHandler.respondToPopup(id, inputValues, button, m_orchestrator)) {
        return makeToolResult(R"({"ok":true})");
    }
    return makeToolResult(std::format(R"({{"error":"Popup not found with id: {}"}})", id), true);
}

nlohmann::json McpHttpServerLayer::handleAbort(const nlohmann::json& /*args*/)
{
    if (!m_running) { return makeToolResult(R"({"error":"No tests are running"})", true); }

    // TODO: Implement orchestrator abort mechanism
    // For now, just report that abort was requested
    return makeToolResult(R"({"error":"Abort not yet implemented"})", true);
}

nlohmann::json McpHttpServerLayer::handleListNodes(const nlohmann::json& /*args*/)
{
    auto&          nodes    = m_canOpen.getNodes();
    nlohmann::json nodeList = nlohmann::json::array();
    for (const auto& n : nodes) {
        nodeList.push_back({{"id", n.nodeId()}, {"name", n.name()}});
    }
    return makeToolResult(nodeList.dump());
}

nlohmann::json McpHttpServerLayer::handleListDevices(const nlohmann::json& /*args*/)
{
    nlohmann::json deviceList = nlohmann::json::array();
    for (const auto& [name, _] : m_canOpen.m_devices.devices) {
        deviceList.push_back({{"name", name}});
    }
    return makeToolResult(deviceList.dump());
}

nlohmann::json McpHttpServerLayer::handleUploadSdo(const nlohmann::json& args)
{
    uint8_t     nodeId   = 0;
    uint16_t    index    = 0;
    uint8_t     subIndex = 0;
    std::string type;
    try {
        nodeId   = args["nodeId"].get<uint8_t>();
        index    = args["index"].get<uint16_t>();
        subIndex = args["sub-index"].get<uint8_t>();
        type     = args["type"].get<std::string>();
    }
    catch (std::exception&) {
        return makeToolResult(R"({"error":"nodeId, index, sub-index and type are required"})", true);
    }

    auto maybeNode = m_canOpen.getNode(nodeId);
    if (!maybeNode.has_value()) {
        return makeToolResult(nlohmann::json({{"error", "Node not found"}, {"nodeId", nodeId}}).dump(), true);
    }

    CanOpen::VarType varType = CanOpen::VarType::Undefined;
    if (type == "bool") { varType = CanOpen::VarType::Boolean; }
    else if (type == "signed8") { varType = CanOpen::VarType::Signed8; }
    else if (type == "signed16") { varType = CanOpen::VarType::Signed16; }
    else if (type == "signed32") { varType = CanOpen::VarType::Signed32; }
    else if (type == "signed64") { varType = CanOpen::VarType::Signed64; }
    else if (type == "unsigned8") { varType = CanOpen::VarType::Unsigned8; }
    else if (type == "unsigned16") { varType = CanOpen::VarType::Unsigned16; }
    else if (type == "unsigned32") { varType = CanOpen::VarType::Unsigned32; }
    else if (type == "unsigned64") { varType = CanOpen::VarType::Unsigned64; }
    else if (type == "real32") { varType = CanOpen::VarType::Real32; }
    else if (type == "real64") { varType = CanOpen::VarType::Real64; }
    else if (type == "string") { varType = CanOpen::VarType::String; }

    if (varType == CanOpen::VarType::Undefined) {
        return makeToolResult(nlohmann::json({{"error", "Invalid type"}, {"type", type}}).dump(), true);
    }

    auto& node = maybeNode.value();
    auto* sdo  = node->sdoInterface();

    auto request = sdo->uploadData(index, subIndex, 500, 3, false, varType);
    auto result  = request.future.get();

    if (request.status() != CanOpen::SdoRequestStatus::Complete &&
        request.status() != CanOpen::SdoRequestStatus::Cancelled) {
        return makeToolResult(
          nlohmann::json({{"error", "SDO upload failed"}, {"status", std::format("{}", request.status())}}).dump(),
          true);
    }

    if (!result.has_value()) {
        return makeToolResult(
          nlohmann::json({{"error", "SDO upload failed"},
                          {"status", std::format("{}", result.error())},
                          {"extra", std::format("{}", request.abortCode())}})
            .dump(),
          true);
    }

    auto& v = result.value();
    switch (varType) {
        case CanOpen::VarType::Boolean:
            return makeToolResult(nlohmann::json({{"value", Deserialize<bool>(v.begin(), v.end())}}).dump());
        case CanOpen::VarType::Signed8:
            return makeToolResult(nlohmann::json({{"value", Deserialize<int8_t>(v.begin(), v.end())}}).dump());
        case CanOpen::VarType::Signed16:
            return makeToolResult(nlohmann::json({{"value", Deserialize<int16_t>(v.begin(), v.end())}}).dump());
        case CanOpen::VarType::Signed32:
            return makeToolResult(nlohmann::json({{"value", Deserialize<int32_t>(v.begin(), v.end())}}).dump());
        case CanOpen::VarType::Signed64:
            return makeToolResult(nlohmann::json({{"value", Deserialize<int64_t>(v.begin(), v.end())}}).dump());
        case CanOpen::VarType::Unsigned8:
            return makeToolResult(nlohmann::json({{"value", Deserialize<uint8_t>(v.begin(), v.end())}}).dump());
        case CanOpen::VarType::Unsigned16:
            return makeToolResult(nlohmann::json({{"value", Deserialize<uint16_t>(v.begin(), v.end())}}).dump());
        case CanOpen::VarType::Unsigned32:
            return makeToolResult(nlohmann::json({{"value", Deserialize<uint32_t>(v.begin(), v.end())}}).dump());
        case CanOpen::VarType::Unsigned64:
            return makeToolResult(nlohmann::json({{"value", Deserialize<uint64_t>(v.begin(), v.end())}}).dump());
        case CanOpen::VarType::Real32:
            return makeToolResult(nlohmann::json({{"value", Deserialize<float>(v.begin(), v.end())}}).dump());
        case CanOpen::VarType::Real64:
            return makeToolResult(nlohmann::json({{"value", Deserialize<double>(v.begin(), v.end())}}).dump());
        case CanOpen::VarType::String:
            return makeToolResult(
              nlohmann::json({{"value", std::string(reinterpret_cast<const char*>(v.data()), v.size())}}).dump());
        default:
            return makeToolResult(R"({"error":"Invalid type"})", true);
    }
}

}    // namespace Frasy
