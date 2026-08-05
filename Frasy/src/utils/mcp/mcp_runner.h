/**
 * @file    mcp_runner.h
 * @author  Frasy
 * @date    2026-08-05
 * @brief   MCP runner - drives the orchestrator and registers MCP tools.
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
#ifndef FRASY_UTILS_MCP_RUNNER_H
#define FRASY_UTILS_MCP_RUNNER_H

#include "mcp_popup_handler.h"
#include "mcp_server.h"
#include "utils/cli/cli_args.h"
#include "utils/communication/can_open/can_open.h"
#include "utils/headless/product_provider.h"
#include "utils/lua/orchestrator/orchestrator.h"

#include <atomic>
#include <string>
#include <vector>

namespace Frasy::Mcp {

/**
 * @brief Drives the MCP server mode — registers tools and runs the MCP message loop.
 */
class McpRunner {
public:
    McpRunner(Headless::ProductProvider& provider);

    /// Run the MCP server loop. Blocks until stdin closes. Returns exit code.
    int run();

private:
    struct ProductInfo {
        std::string environmentPath;
        std::string testPath;
        std::string name;
    };

    std::vector<ProductInfo> discoverProducts();
    void                     registerTools();

    // Tool handlers
    nlohmann::json handleListProducts(const nlohmann::json& args);
    nlohmann::json handleRunTests(const nlohmann::json& args);
    nlohmann::json handleGetStatus(const nlohmann::json& args);
    nlohmann::json handleGetResults(const nlohmann::json& args);
    nlohmann::json handleGetPendingPopup(const nlohmann::json& args);
    nlohmann::json handleRespondToPopup(const nlohmann::json& args);

    nlohmann::json makeToolResult(const std::string& text, bool isError = false);

    Headless::ProductProvider& m_provider;
    Lua::Orchestrator          m_orchestrator;
    CanOpen::CanOpen           m_canOpen;
    McpServer                  m_server;
    McpPopupHandler            m_popupHandler;

    std::atomic<bool>        m_running = false;
    std::vector<std::string> m_serials;
    std::string              m_activeProduct;
};

}    // namespace Frasy::Mcp

#endif    // FRASY_UTILS_MCP_RUNNER_H
