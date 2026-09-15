/**
 * @file    unified_popup_handler.h
 * @author  Frasy
 * @date    2026-08-17
 * @brief   Dual-path popup handler — observes orchestrator popups for MCP access.
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
#ifndef FRASY_UTILS_MCP_UNIFIED_POPUP_HANDLER_H
#define FRASY_UTILS_MCP_UNIFIED_POPUP_HANDLER_H

#include "utils/lua/orchestrator/orchestrator.h"

#include <json.hpp>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace Frasy::Mcp {

/**
 * @brief Observes the orchestrator's popup map and exposes popups to MCP clients.
 *
 * The GUI continues to render popups via orchestrator.renderPopups() as normal.
 * This handler sits alongside, detecting new/consumed popups each frame via sync().
 * MCP clients can query pending popups and respond to them — first responder wins.
 */
class UnifiedPopupHandler {
public:
    struct TrackedPopup {
        std::string              id;
        std::vector<std::string> texts;
        std::vector<std::string> inputTitles;
        std::vector<std::string> buttons;
    };

    /// Called each frame from onUpdate(). Detects new and consumed popups.
    /// Must be called on the main thread (same thread as renderPopups).
    void sync(Lua::Orchestrator& orchestrator);

    /// Get the next pending popup as JSON for the MCP client.
    /// Thread-safe (called from HTTP handler thread).
    std::optional<nlohmann::json> getPendingPopup();

    /// Respond to a popup by ID. Sets inputs and clicks a button.
    /// Thread-safe (called from HTTP handler thread).
    /// Returns true if the popup was found and responded to.
    bool respondToPopup(const std::string&                    id,
                        const std::map<std::size_t, std::string>& inputs,
                        const std::string&                    button,
                        Lua::Orchestrator&                    orchestrator);

    /// Get list of popup IDs that were newly added since last call.
    /// Used for SSE notifications. Clears the list after returning.
    std::vector<TrackedPopup> consumeNewPopups();

    /// Get list of popup IDs that were consumed since last call.
    /// Used for SSE notifications. Clears the list after returning.
    std::vector<std::string> consumeRemovedPopups();

private:
    std::mutex                 m_mutex;
    std::vector<TrackedPopup>  m_tracked;
    std::set<std::string>      m_knownPopupIds;

    // For SSE notifications
    std::vector<TrackedPopup>  m_newPopups;
    std::vector<std::string>   m_removedPopups;
};

}    // namespace Frasy::Mcp

#endif    // FRASY_UTILS_MCP_UNIFIED_POPUP_HANDLER_H
