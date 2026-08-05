/**
 * @file    mcp_popup_handler.h
 * @author  Frasy
 * @date    2026-08-05
 * @brief   MCP popup handler - queues popups for retrieval via MCP tools.
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
#ifndef FRASY_UTILS_MCP_POPUP_HANDLER_H
#define FRASY_UTILS_MCP_POPUP_HANDLER_H

#include <condition_variable>
#include <json.hpp>
#include <mutex>
#include <optional>
#include <queue>
#include <sol/sol.hpp>
#include <string>
#include <vector>

namespace Frasy::Mcp {

/**
 * @brief A pending popup waiting for an MCP response.
 *
 * Created when a Lua thread triggers a popup during execution.
 * The Lua thread blocks on the condition variable until respond() is called.
 */
struct PendingPopup {
    std::string              id;
    std::string              name;
    std::size_t              uut;
    std::vector<std::string> texts;
    std::vector<std::string> inputTitles;
    std::vector<std::string> inputValues;
    std::vector<std::string> buttons;
    std::vector<bool>        buttonConsumes;

    // Synchronization: the Lua thread waits on this
    std::mutex              mutex;
    std::condition_variable cv;
    bool                    responded = false;

    // Response data (filled by respond_to_popup)
    std::string              responseButton;
    std::map<int, std::string> responseInputs;
};

/**
 * @brief Manages the popup queue for MCP mode.
 *
 * Lua threads push popups here and block. The MCP server polls for pending
 * popups and sends responses, unblocking the Lua threads.
 */
class McpPopupHandler {
public:
    McpPopupHandler() = default;

    /// Install the MCP popup import into a Lua state (called by orchestrator's initLua).
    void importPopup(sol::state_view lua, std::size_t uut);

    /// Get the next pending popup (for get_pending_popup tool). Returns nullopt if none.
    std::optional<nlohmann::json> getPendingPopup();

    /// Respond to a popup by ID (for respond_to_popup tool). Returns false if popup not found.
    bool respondToPopup(const std::string& id, const std::map<int, std::string>& inputs, const std::string& button);

private:
    std::mutex                             m_queueMutex;
    std::queue<std::shared_ptr<PendingPopup>> m_queue;
    std::vector<std::shared_ptr<PendingPopup>> m_active;    // popups currently waiting for response
};

}    // namespace Frasy::Mcp

#endif    // FRASY_UTILS_MCP_POPUP_HANDLER_H
