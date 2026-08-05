/**
 * @file    mcp_popup_handler.cpp
 * @author  Frasy
 * @date    2026-08-05
 * @brief   MCP popup handler implementation.
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
#include "mcp_popup_handler.h"

#include <Brigerad/Core/Log.h>

#include <algorithm>
#include <format>

namespace Frasy::Mcp {

namespace {
constexpr auto s_tag = "MCP-Popup";

enum class ElementKind : uint8_t {
    Text            = 0,
    TextDynamic     = 1,
    Input           = 2,
    Button          = 3,
    Image           = 4,
    BeginHorizontal = 5,
    EndHorizontal   = 6,
    BeginVertical   = 7,
    EndVertical     = 8,
    SameLine        = 9,
    Spring          = 10,
};
}    // namespace

void McpPopupHandler::importPopup(sol::state_view lua, std::size_t uut)
{
    lua.script_file("lua/core/sdk/popup.lua");
    lua["__popup"] = lua.create_table();

    // Consume is a no-op — we handle consumption in respondToPopup
    lua["__popup"]["Consume"] = [](sol::table) {};

    if (lua["Context"]["info"]["stage"].get<int>() == 3) {    // Stage::execution
        lua["__popup"]["Show"] = [this, uut](sol::table builder) -> std::vector<std::string> {
            // Extract popup info from builder
            auto popup  = std::make_shared<PendingPopup>();
            popup->name = builder["name"].get_or<std::string>("");
            popup->uut  = uut;
            popup->id   = std::format("popup_UUT{}_{}", uut, popup->name);

            auto elements = builder["elements"].get<std::vector<sol::table>>();
            for (const auto& element : elements) {
                auto kind = element["kind"].get<ElementKind>();
                switch (kind) {
                    case ElementKind::Text:
                        popup->texts.push_back(element["text"].get<std::string>());
                        break;
                    case ElementKind::Input:
                        popup->inputTitles.push_back(element["title"].get<std::string>());
                        popup->inputValues.emplace_back();
                        break;
                    case ElementKind::Button:
                        popup->buttons.push_back(element["label"].get<std::string>());
                        popup->buttonConsumes.push_back(element["consume"].get<bool>());
                        break;
                    default: break;
                }
            }

            // Queue the popup and make it active
            {
                std::lock_guard lock(m_queueMutex);
                m_active.push_back(popup);
            }

            BR_LOG_DEBUG(s_tag, "Popup queued: '{}' (UUT {})", popup->name, uut);

            // Block until response arrives
            {
                std::unique_lock lock(popup->mutex);
                popup->cv.wait(lock, [&popup] { return popup->responded; });
            }

            BR_LOG_DEBUG(s_tag, "Popup responded: '{}' (UUT {})", popup->name, uut);

            // Apply response inputs
            for (auto& [idx, value] : popup->responseInputs) {
                if (idx >= 1 && idx <= static_cast<int>(popup->inputValues.size())) {
                    popup->inputValues[idx - 1] = value;
                }
            }

            // Call button action if applicable
            int buttonIdx = -1;
            auto lower = [](std::string s) {
                std::ranges::transform(s, s.begin(), ::tolower);
                return s;
            };
            std::string lowerButton = lower(popup->responseButton);
            for (size_t i = 0; i < popup->buttons.size(); ++i) {
                if (lower(popup->buttons[i]) == lowerButton) {
                    buttonIdx = static_cast<int>(i);
                    break;
                }
            }

            // Find and call the button action
            if (buttonIdx >= 0) {
                int btnCount = 0;
                for (const auto& element : elements) {
                    if (element["kind"].get<ElementKind>() == ElementKind::Button) {
                        if (btnCount == buttonIdx) {
                            auto action = element["action"].get<sol::unsafe_function>();
                            if (action.valid()) {
                                auto result = action(popup->inputValues);
                                if (!result.valid()) {
                                    sol::error err = result;
                                    BR_LOG_ERROR(s_tag, "Button action error: {}", err.what());
                                }
                            }
                            break;
                        }
                        btnCount++;
                    }
                }
            }

            // Remove from active list
            {
                std::lock_guard lock(m_queueMutex);
                std::erase_if(m_active, [&popup](const auto& p) { return p->id == popup->id; });
            }

            return popup->inputValues;
        };
    }
    else {
        // Generation/Validation — return empty inputs immediately
        lua["__popup"]["Show"] = [uut](sol::table builder) -> std::vector<std::string> {
            std::vector<std::string> inputs;
            auto elements = builder["elements"].get<std::vector<sol::table>>();
            for (const auto& element : elements) {
                if (element["kind"].get<ElementKind>() == ElementKind::Input) {
                    inputs.emplace_back();
                }
            }
            return inputs;
        };
    }
}

std::optional<nlohmann::json> McpPopupHandler::getPendingPopup()
{
    std::lock_guard lock(m_queueMutex);
    if (m_active.empty()) { return std::nullopt; }

    // Return the first active popup that hasn't been responded to yet
    for (const auto& popup : m_active) {
        std::lock_guard pLock(popup->mutex);
        if (!popup->responded) {
            nlohmann::json j;
            j["id"]   = popup->id;
            j["name"] = popup->name;
            j["uut"]  = popup->uut;
            j["texts"] = popup->texts;

            nlohmann::json inputs = nlohmann::json::array();
            for (size_t i = 0; i < popup->inputTitles.size(); ++i) {
                inputs.push_back({{"index", i + 1}, {"title", popup->inputTitles[i]}, {"value", popup->inputValues[i]}});
            }
            j["inputs"] = inputs;

            nlohmann::json buttons = nlohmann::json::array();
            for (const auto& btn : popup->buttons) {
                buttons.push_back(btn);
            }
            j["buttons"] = buttons;

            return j;
        }
    }

    return std::nullopt;
}

bool McpPopupHandler::respondToPopup(const std::string& id, const std::map<int, std::string>& inputs, const std::string& button)
{
    std::lock_guard lock(m_queueMutex);
    for (auto& popup : m_active) {
        if (popup->id == id) {
            {
                std::lock_guard pLock(popup->mutex);
                popup->responseInputs = inputs;
                popup->responseButton = button;
                popup->responded      = true;
            }
            popup->cv.notify_one();
            return true;
        }
    }
    return false;
}

}    // namespace Frasy::Mcp
