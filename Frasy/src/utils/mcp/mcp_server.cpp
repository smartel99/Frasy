/**
 * @file    mcp_server.cpp
 * @author  Frasy
 * @date    2026-08-05
 * @brief   MCP server implementation.
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
#include "mcp_server.h"

#include <Brigerad/Core/Log.h>

#include <iostream>
#include <string>

namespace Frasy::Mcp {

namespace {
constexpr auto s_tag             = "MCP";
constexpr auto s_protocolVersion = "2024-11-05";
}    // namespace

McpServer::McpServer(const std::string& serverName, const std::string& serverVersion)
: m_serverName(serverName), m_serverVersion(serverVersion)
{
}

void McpServer::registerTool(const std::string&    name,
                             const std::string&    description,
                             const nlohmann::json& inputSchema,
                             ToolHandler           handler)
{
    m_tools[name] = ToolDef {name, description, inputSchema, std::move(handler)};
}

void McpServer::run()
{
    BR_LOG_INFO(s_tag, "MCP server started, waiting for JSON-RPC on stdin");

    std::string line;
    while (std::getline(std::cin, line)) {
        // Trim trailing whitespace/CR
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
        if (line.empty()) { continue; }

        try {
            auto msg = nlohmann::json::parse(line);

            // Validate basic JSON-RPC structure
            if (!msg.contains("jsonrpc") || msg["jsonrpc"] != "2.0") {
                BR_LOG_WARN(s_tag, "Received non-JSON-RPC message, ignoring");
                continue;
            }

            // Check if it's a notification (no id) or a request (has id)
            bool isNotification = !msg.contains("id");
            auto id             = isNotification ? nlohmann::json(nullptr) : msg["id"];

            if (!msg.contains("method")) {
                if (!isNotification) { sendError(id, -32600, "Invalid Request: missing method"); }
                continue;
            }

            std::string method = msg["method"];
            auto        params = msg.value("params", nlohmann::json::object());

            // Handle notifications (no response expected)
            if (method == "notifications/initialized") {
                BR_LOG_INFO(s_tag, "Client initialized");
                continue;
            }
            if (method == "notifications/cancelled") {
                BR_LOG_WARN(s_tag, "Client cancelled request");
                continue;
            }
            if (isNotification) {
                // Unknown notification — ignore per spec
                continue;
            }

            // Handle requests
            if (method == "initialize") {
                auto result = handleInitialize(params);
                sendResponse(id, result);
            }
            else if (method == "tools/list") {
                auto result = handleToolsList();
                sendResponse(id, result);
            }
            else if (method == "tools/call") {
                auto result = handleToolsCall(params);
                sendResponse(id, result);
            }
            else if (method == "ping") {
                sendResponse(id, nlohmann::json::object());
            }
            else {
                sendError(id, -32601, "Method not found: " + method);
            }
        }
        catch (const nlohmann::json::exception& e) {
            BR_LOG_ERROR(s_tag, "JSON parse error: {}", e.what());
            // Can't send error response without an id
            sendError(nullptr, -32700, "Parse error");
        }
        catch (const std::exception& e) {
            BR_LOG_ERROR(s_tag, "Internal error: {}", e.what());
        }
    }

    BR_LOG_INFO(s_tag, "stdin closed, MCP server shutting down");
}

void McpServer::sendResponse(const nlohmann::json& id, const nlohmann::json& result)
{
    nlohmann::json response;
    response["jsonrpc"] = "2.0";
    response["id"]      = id;
    response["result"]  = result;
    std::cout << response.dump() << "\n" << std::flush;
}

void McpServer::sendError(const nlohmann::json& id, int code, const std::string& message)
{
    nlohmann::json response;
    response["jsonrpc"]        = "2.0";
    response["id"]             = id;
    response["error"]["code"]  = code;
    response["error"]["message"] = message;
    std::cout << response.dump() << "\n" << std::flush;
}

void McpServer::sendToolResult(const nlohmann::json& id, const std::string& text, bool isError)
{
    nlohmann::json result;
    result["content"] = nlohmann::json::array({{{"type", "text"}, {"text", text}}});
    result["isError"] = isError;
    sendResponse(id, result);
}

nlohmann::json McpServer::handleInitialize(const nlohmann::json& params)
{
    BR_LOG_INFO(s_tag, "Client connecting: {}",
                params.value("clientInfo", nlohmann::json::object()).value("name", "unknown"));

    m_initialized = true;

    nlohmann::json result;
    result["protocolVersion"]          = s_protocolVersion;
    result["capabilities"]["tools"]    = nlohmann::json::object();
    result["serverInfo"]["name"]       = m_serverName;
    result["serverInfo"]["version"]    = m_serverVersion;
    return result;
}

nlohmann::json McpServer::handleToolsList()
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

nlohmann::json McpServer::handleToolsCall(const nlohmann::json& params)
{
    std::string name = params.value("name", "");
    auto        args = params.value("arguments", nlohmann::json::object());

    auto it = m_tools.find(name);
    if (it == m_tools.end()) {
        nlohmann::json result;
        result["content"] = nlohmann::json::array({{{"type", "text"}, {"text", "Unknown tool: " + name}}});
        result["isError"] = true;
        return result;
    }

    try {
        return it->second.handler(args);
    }
    catch (const std::exception& e) {
        nlohmann::json result;
        result["content"] = nlohmann::json::array({{{"type", "text"}, {"text", std::string("Tool error: ") + e.what()}}});
        result["isError"] = true;
        return result;
    }
}

}    // namespace Frasy::Mcp
