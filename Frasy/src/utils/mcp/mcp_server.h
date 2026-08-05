/**
 * @file    mcp_server.h
 * @author  Frasy
 * @date    2026-08-05
 * @brief   MCP (Model Context Protocol) server over stdio for AI agent interaction.
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
#ifndef FRASY_UTILS_MCP_SERVER_H
#define FRASY_UTILS_MCP_SERVER_H

#include <functional>
#include <json.hpp>
#include <string>
#include <unordered_map>

namespace Frasy::Mcp {

/**
 * @brief MCP stdio server implementing the Model Context Protocol.
 *
 * Reads JSON-RPC messages from stdin (newline-delimited), dispatches tool calls
 * to registered handlers, and writes responses to stdout.
 */
class McpServer {
public:
    using ToolHandler = std::function<nlohmann::json(const nlohmann::json& arguments)>;

    McpServer(const std::string& serverName, const std::string& serverVersion);

    /// Register a tool with its schema and handler.
    void registerTool(const std::string&    name,
                      const std::string&    description,
                      const nlohmann::json& inputSchema,
                      ToolHandler           handler);

    /// Run the main message loop. Blocks until stdin is closed.
    void run();

private:
    void sendResponse(const nlohmann::json& id, const nlohmann::json& result);
    void sendError(const nlohmann::json& id, int code, const std::string& message);
    void sendToolResult(const nlohmann::json& id, const std::string& text, bool isError = false);

    nlohmann::json handleInitialize(const nlohmann::json& params);
    nlohmann::json handleToolsList();
    nlohmann::json handleToolsCall(const nlohmann::json& params);

    struct ToolDef {
        std::string    name;
        std::string    description;
        nlohmann::json inputSchema;
        ToolHandler    handler;
    };

    std::string                                  m_serverName;
    std::string                                  m_serverVersion;
    std::unordered_map<std::string, ToolDef>     m_tools;
    bool                                         m_initialized = false;
};

}    // namespace Frasy::Mcp

#endif    // FRASY_UTILS_MCP_SERVER_H
