/**
 * @file    mcp_relay.h
 * @author  Frasy
 * @date    2026-08-17
 * @brief   MCP client relay — bridges stdio (AI agent) to HTTP (running Frasy instance).
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
#ifndef FRASY_UTILS_MCP_RELAY_H
#define FRASY_UTILS_MCP_RELAY_H

#include <atomic>
#include <string>
#include <thread>

namespace Frasy::Mcp {

/**
 * @brief Lightweight stdio ↔ HTTP relay for connecting AI agents to a running Frasy instance.
 *
 * Reads JSON-RPC messages from stdin, POSTs them to the primary instance's /mcp endpoint,
 * and writes responses to stdout. Optionally maintains a background SSE connection for
 * server-initiated notifications.
 *
 * This process does NOT create an Application or initialize OpenGL.
 */
class McpRelay {
public:
    McpRelay(const std::string& address, int port);

    /// Run the relay loop. Blocks until stdin closes. Returns exit code.
    int run();

private:
    void sseListenerThread();

    std::string      m_url;         // e.g., "http://127.0.0.1:8080/mcp"
    std::string      m_address;
    int              m_port;
    std::string      m_sessionId;
    std::jthread     m_sseThread;
    std::atomic<bool> m_running = true;
};

}    // namespace Frasy::Mcp

#endif    // FRASY_UTILS_MCP_RELAY_H
