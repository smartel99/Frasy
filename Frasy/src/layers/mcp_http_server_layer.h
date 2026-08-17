/**
 * @file    mcp_http_server_layer.h
 * @author  Frasy
 * @date    2026-08-17
 * @brief   MCP HTTP Server Layer — hosts MCP Streamable HTTP transport in the GUI process.
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
#ifndef FRASY_LAYERS_MCP_HTTP_SERVER_LAYER_H
#define FRASY_LAYERS_MCP_HTTP_SERVER_LAYER_H

#include "utils/communication/can_open/can_open.h"
#include "utils/headless/product_provider.h"
#include "utils/lua/orchestrator/orchestrator.h"
#include "utils/mcp/unified_popup_handler.h"
#include "utils/run_owner.h"

#include <Brigerad.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <json.hpp>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

// Forward-declare httplib types to avoid including the large header here
namespace httplib {
class Server;
class Request;
class Response;
}    // namespace httplib

namespace Frasy {

/**
 * @brief Brigerad Layer that hosts an MCP HTTP server alongside the GUI.
 *
 * Shares the orchestrator and CanOpen instances with the GUI's MainApplicationLayer.
 * Implements MCP Streamable HTTP transport (POST /mcp for tool calls, GET /mcp for SSE).
 * Mutating operations are dispatched to the main thread via a command queue.
 */
class McpHttpServerLayer : public Brigerad::Layer {
public:
    McpHttpServerLayer(int                       port,
                       Lua::Orchestrator&        orchestrator,
                       CanOpen::CanOpen&         canOpen,
                       Headless::ProductProvider& provider);
    ~McpHttpServerLayer() override;

    void onAttach() override;
    void onDetach() override;
    void onUpdate(Brigerad::Timestep ts) override;

    /// Get the actual port the server is listening on (useful when port 0 = auto).
    [[nodiscard]] int getListeningPort() const { return m_listeningPort; }

    /// Get the current run owner.
    [[nodiscard]] RunOwner getRunOwner() const { return m_runOwner.load(); }

    /// Set the run owner (called by GUI when operator starts a run).
    void setRunOwner(RunOwner owner) { m_runOwner.store(owner); }

    /// Set callbacks for product state (called during integration wiring).
    using ActiveProductGetter = std::function<std::string()>;
    using ProductLoader       = std::function<bool(const std::string&)>;

    void setProductCallbacks(ActiveProductGetter getter, ProductLoader loader)
    {
        m_getActiveProduct = std::move(getter);
        m_loadProduct      = std::move(loader);
    }

private:
    // --- HTTP handlers ---
    void handlePost(const httplib::Request& req, httplib::Response& res);
    void handleGet(const httplib::Request& req, httplib::Response& res);
    void handleDelete(const httplib::Request& req, httplib::Response& res);

    // --- JSON-RPC dispatch ---
    nlohmann::json dispatch(const nlohmann::json& message);
    nlohmann::json handleInitialize(const nlohmann::json& params);
    nlohmann::json handleToolsList();
    nlohmann::json handleToolsCall(const nlohmann::json& params);

    // --- Tool handlers ---
    using ToolHandler = std::function<nlohmann::json(const nlohmann::json&)>;

    struct ToolDef {
        std::string    name;
        std::string    description;
        nlohmann::json inputSchema;
        ToolHandler    handler;
    };

    void registerTools();

    nlohmann::json handleListProducts(const nlohmann::json& args);
    nlohmann::json handleLoadProduct(const nlohmann::json& args);
    nlohmann::json handleRunTests(const nlohmann::json& args);
    nlohmann::json handleGetStatus(const nlohmann::json& args);
    nlohmann::json handleGetResults(const nlohmann::json& args);
    nlohmann::json handleGetPendingPopup(const nlohmann::json& args);
    nlohmann::json handleRespondToPopup(const nlohmann::json& args);
    nlohmann::json handleAbort(const nlohmann::json& args);
    nlohmann::json handleListNodes(const nlohmann::json& args);
    nlohmann::json handleListDevices(const nlohmann::json& args);
    nlohmann::json handleUploadSdo(const nlohmann::json& args);

    // --- Helpers ---
    static nlohmann::json makeToolResult(const std::string& text, bool isError = false);
    static nlohmann::json makeJsonRpcResponse(const nlohmann::json& id, const nlohmann::json& result);
    static nlohmann::json makeJsonRpcError(const nlohmann::json& id, int code, const std::string& message);

    std::vector<Headless::ProductInfo> discoverProducts();

    // --- Command queue (main-thread execution of mutating ops) ---
    struct Command {
        std::function<nlohmann::json()> execute;
        std::promise<nlohmann::json>    result;
    };

    nlohmann::json enqueueAndWait(std::function<nlohmann::json()> fn);

    std::mutex         m_commandMutex;
    std::queue<Command> m_commandQueue;

    // --- Server state ---
    std::unique_ptr<httplib::Server> m_httpServer;
    std::jthread                     m_serverThread;
    int                              m_requestedPort  = 0;
    int                              m_listeningPort  = 0;

    // --- Shared state (references to GUI's instances) ---
    Lua::Orchestrator&         m_orchestrator;
    CanOpen::CanOpen&          m_canOpen;
    Headless::ProductProvider& m_provider;

    // --- Tool registry ---
    std::unordered_map<std::string, ToolDef> m_tools;

    // --- Session ---
    std::string m_sessionId;
    bool        m_initialized = false;

    // --- Run ownership ---
    std::atomic<RunOwner> m_runOwner = RunOwner::None;
    std::atomic<bool>     m_running  = false;
    std::string           m_activeProduct;
    std::vector<std::string> m_serials;

    // --- Product state callbacks ---
    ActiveProductGetter m_getActiveProduct;
    ProductLoader       m_loadProduct;

    // --- Popup handler ---
    Mcp::UnifiedPopupHandler m_popupHandler;

    // --- SSE connections ---
    struct SseConnection {
        std::mutex              mutex;
        std::queue<std::string> events;
        std::condition_variable cv;
        std::atomic<bool>       closed = false;
    };
    std::mutex                                    m_sseMutex;
    std::vector<std::shared_ptr<SseConnection>>   m_sseConnections;

    void pushSseEvent(const nlohmann::json& event);
    void cleanClosedSseConnections();

    static constexpr const char* s_tag = "MCP-HTTP";
};

}    // namespace Frasy

#endif    // FRASY_LAYERS_MCP_HTTP_SERVER_LAYER_H
