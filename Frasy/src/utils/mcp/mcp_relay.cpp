/**
 * @file    mcp_relay.cpp
 * @author  Frasy
 * @date    2026-08-17
 * @brief   MCP client relay implementation.
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
#include "mcp_relay.h"

#include <httplib.h>

#include <iostream>
#include <mutex>
#include <string>

namespace Frasy::Mcp {

namespace {
// Mutex to serialize stdout writes (main thread + SSE thread both write)
std::mutex s_stdoutMutex;
}    // namespace

McpRelay::McpRelay(const std::string& address, int port)
: m_address(address), m_port(port)
{
    m_url = std::format("http://{}:{}/mcp", address, port);
}

int McpRelay::run()
{
    std::cerr << "[mcp-client] Connecting to " << m_url << "\n";

    httplib::Client client(m_address, m_port);
    client.set_connection_timeout(5);
    client.set_read_timeout(30);
    client.set_write_timeout(5);

    // Start SSE listener thread for server-initiated notifications
    m_sseThread = std::jthread([this](std::stop_token) { sseListenerThread(); });

    // Main loop: read JSON-RPC from stdin, POST to server, write response to stdout
    std::string line;
    while (std::getline(std::cin, line)) {
        // Trim trailing whitespace/CR
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
        if (line.empty()) { continue; }

        // Build request headers
        httplib::Headers headers = {
          {"Content-Type", "application/json"},
          {"Accept", "application/json, text/event-stream"},
        };
        if (!m_sessionId.empty()) {
            headers.emplace("Mcp-Session-Id", m_sessionId);
        }

        // POST to server
        auto result = client.Post("/mcp", headers, line, "application/json");

        if (!result) {
            // Connection error — write a JSON-RPC error to stdout
            std::string error = R"({"jsonrpc":"2.0","id":null,"error":{"code":-32000,"message":"Connection failed to )"
                              + m_url + R"("}})";
            std::lock_guard lock(s_stdoutMutex);
            std::cout << error << "\n" << std::flush;
            continue;
        }

        // Capture session ID from initialize response
        if (result->has_header("Mcp-Session-Id")) {
            m_sessionId = result->get_header_value("Mcp-Session-Id");
        }

        // Write response to stdout
        {
            std::lock_guard lock(s_stdoutMutex);
            std::cout << result->body << "\n" << std::flush;
        }
    }

    // stdin closed — shut down
    m_running = false;
    std::cerr << "[mcp-client] stdin closed, shutting down\n";

    if (m_sseThread.joinable()) {
        m_sseThread.request_stop();
        m_sseThread.join();
    }

    return 0;
}

void McpRelay::sseListenerThread()
{
    // Give the main thread a moment to establish the session
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    httplib::Client sseClient(m_address, m_port);
    sseClient.set_connection_timeout(5);
    sseClient.set_read_timeout(0);    // Infinite timeout for SSE

    httplib::Headers headers = {
      {"Accept", "text/event-stream"},
    };
    if (!m_sessionId.empty()) {
        headers.emplace("Mcp-Session-Id", m_sessionId);
    }

    // Use a streaming GET to receive SSE events
    sseClient.Get("/mcp", headers,
        [this](const httplib::Response& response) {
            // Check we got SSE content type
            auto ct = response.get_header_value("Content-Type");
            return ct.find("text/event-stream") != std::string::npos && m_running;
        },
        [this](const char* data, size_t len) {
            if (!m_running) { return false; }

            // Parse SSE data lines and forward to stdout
            // SSE format: "data: {...}\n\n"
            std::string chunk(data, len);
            std::istringstream stream(chunk);
            std::string sseDataLine;

            while (std::getline(stream, sseDataLine)) {
                // Trim CR
                if (!sseDataLine.empty() && sseDataLine.back() == '\r') {
                    sseDataLine.pop_back();
                }

                // Extract "data: " prefix
                if (sseDataLine.starts_with("data: ")) {
                    std::string jsonPayload = sseDataLine.substr(6);
                    if (!jsonPayload.empty()) {
                        std::lock_guard lock(s_stdoutMutex);
                        std::cout << jsonPayload << "\n" << std::flush;
                    }
                }
                // Ignore other SSE fields (id:, event:, retry:, empty lines)
            }
            return m_running.load();
        });

    if (m_running) {
        std::cerr << "[mcp-client] SSE connection closed\n";
    }
}

}    // namespace Frasy::Mcp
