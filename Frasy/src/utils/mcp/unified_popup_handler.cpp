/**
 * @file    unified_popup_handler.cpp
 * @author  Frasy
 * @date    2026-08-17
 * @brief   Unified popup handler implementation.
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
#include "unified_popup_handler.h"

#include "utils/lua/popup.h"

#include <Brigerad/Core/Log.h>

#include <algorithm>

namespace Frasy::Mcp {

namespace {
constexpr auto s_tag = "MCP-Popup";

UnifiedPopupHandler::TrackedPopup extractPopupInfo(const std::string& id, const Lua::Popup& popup)
{
    UnifiedPopupHandler::TrackedPopup tracked;
    tracked.id = id;

    for (const auto& elem : popup.getElements()) {
        switch (elem->kind) {
            case Lua::Popup::Element::Kind::Text: {
                auto* t = static_cast<const Lua::Popup::Text*>(elem.get());
                tracked.texts.push_back(t->text);
                break;
            }
            case Lua::Popup::Element::Kind::Input: {
                auto* inp = static_cast<const Lua::Popup::Input*>(elem.get());
                tracked.inputTitles.push_back(inp->title);
                break;
            }
            case Lua::Popup::Element::Kind::Button: {
                auto* btn = static_cast<const Lua::Popup::Button*>(elem.get());
                tracked.buttons.push_back(btn->label);
                break;
            }
            default: break;
        }
    }

    return tracked;
}
}    // namespace

void UnifiedPopupHandler::sync(Lua::Orchestrator& orchestrator)
{
    std::lock_guard popupLock(*orchestrator.getPopupMutex());
    auto&           popups = orchestrator.getPopups();

    std::lock_guard lock(m_mutex);

    // Detect new popups
    for (auto& [name, popup] : popups) {
        if (!m_knownPopupIds.contains(name)) {
            m_knownPopupIds.insert(name);
            auto tracked = extractPopupInfo(name, popup);
            m_tracked.push_back(tracked);
            m_newPopups.push_back(tracked);
            BR_LOG_DEBUG(s_tag, "New popup detected: '{}'", name);
        }
    }

    // Detect consumed popups (removed from the orchestrator's map)
    std::erase_if(m_tracked, [&](const TrackedPopup& tp) {
        if (!popups.contains(tp.id)) {
            m_removedPopups.push_back(tp.id);
            BR_LOG_DEBUG(s_tag, "Popup consumed: '{}'", tp.id);
            return true;
        }
        return false;
    });

    // Clean known set
    std::erase_if(m_knownPopupIds, [&](const std::string& id) {
        return !popups.contains(id);
    });
}

std::optional<nlohmann::json> UnifiedPopupHandler::getPendingPopup()
{
    std::lock_guard lock(m_mutex);

    if (m_tracked.empty()) { return std::nullopt; }

    // Return the first tracked popup
    const auto& popup = m_tracked.front();

    nlohmann::json j;
    j["id"]   = popup.id;
    j["name"] = popup.id;

    nlohmann::json texts = nlohmann::json::array();
    for (const auto& t : popup.texts) { texts.push_back(t); }
    j["texts"] = texts;

    nlohmann::json inputs = nlohmann::json::array();
    for (std::size_t i = 0; i < popup.inputTitles.size(); ++i) {
        inputs.push_back({{"index", i}, {"title", popup.inputTitles[i]}, {"value", ""}});
    }
    j["inputs"] = inputs;

    nlohmann::json buttons = nlohmann::json::array();
    for (const auto& b : popup.buttons) { buttons.push_back(b); }
    j["buttons"] = buttons;

    return j;
}

bool UnifiedPopupHandler::respondToPopup(const std::string&                         id,
                                         const std::map<std::size_t, std::string>&  inputs,
                                         const std::string&                         button,
                                         Lua::Orchestrator&                         orchestrator)
{
    std::lock_guard popupLock(*orchestrator.getPopupMutex());
    auto&           popups = orchestrator.getPopups();
    auto            it     = popups.find(id);
    if (it == popups.end()) { return false; }

    auto& popup = it->second;

    // Set input values
    for (auto& [idx, value] : inputs) {
        popup.setInput(idx, value);
    }

    // Click the button (calls action + consume if button.consume is true)
    if (!popup.clickButton(button)) {
        // Button not found — just consume the popup anyway
        popup.Consume();
    }

    BR_LOG_INFO(s_tag, "Popup '{}' responded via MCP (button: '{}')", id, button);
    return true;
}

std::vector<UnifiedPopupHandler::TrackedPopup> UnifiedPopupHandler::consumeNewPopups()
{
    std::lock_guard lock(m_mutex);
    auto            result = std::move(m_newPopups);
    m_newPopups.clear();
    return result;
}

std::vector<std::string> UnifiedPopupHandler::consumeRemovedPopups()
{
    std::lock_guard lock(m_mutex);
    auto            result = std::move(m_removedPopups);
    m_removedPopups.clear();
    return result;
}

}    // namespace Frasy::Mcp
