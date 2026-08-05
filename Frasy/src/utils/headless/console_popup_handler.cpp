/**
 * @file    console_popup_handler.cpp
 * @author  Frasy
 * @date    2026-08-05
 * @brief   Console-based popup handler implementation.
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
#include "console_popup_handler.h"

#include <Brigerad/Core/Log.h>

#include <algorithm>
#include <chrono>
#include <format>
#include <iostream>
#include <json.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace Frasy::Headless {

namespace {
constexpr auto s_tag = "Popup";

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

struct PopupInfo {
    std::string              name;
    std::size_t              uut;
    std::vector<std::string> texts;
    std::vector<std::string> inputTitles;
    std::vector<std::string> inputValues;
    std::vector<std::string> buttons;
    std::vector<bool>        buttonConsumes;
    std::vector<sol::unsafe_function> buttonActions;
    std::string              consumeButtonText;

    static PopupInfo extract(std::size_t uut, const sol::table& builder)
    {
        PopupInfo info;
        info.uut               = uut;
        info.name              = builder["name"].get_or<std::string>("");
        info.consumeButtonText = builder["consumeButtonText"].get_or<std::string>("Cancel");

        auto elements = builder["elements"].get<std::vector<sol::table>>();
        for (const auto& element : elements) {
            auto kind = element["kind"].get<ElementKind>();
            switch (kind) {
                case ElementKind::Text: info.texts.push_back(element["text"].get<std::string>()); break;
                case ElementKind::Input:
                    info.inputTitles.push_back(element["title"].get<std::string>());
                    info.inputValues.emplace_back();
                    break;
                case ElementKind::Button:
                    info.buttons.push_back(element["label"].get<std::string>());
                    info.buttonConsumes.push_back(element["consume"].get<bool>());
                    info.buttonActions.push_back(element["action"].get<sol::unsafe_function>());
                    break;
                default: break;    // Ignore layout elements (images, horizontal/vertical, etc.)
            }
        }
        return info;
    }
};

void presentHuman(const PopupInfo& info)
{
    std::cout << "\n";
    std::cout << std::format("{:=<50}\n", "");
    std::cout << std::format(" POPUP: \"{}\" (UUT {})\n", info.name, info.uut);
    std::cout << std::format("{:-<50}\n", "");
    for (const auto& text : info.texts) {
        std::cout << " " << text << "\n";
    }
    if (!info.inputTitles.empty()) {
        std::cout << "\n Inputs:\n";
        for (size_t i = 0; i < info.inputTitles.size(); ++i) {
            std::cout << std::format("   [{}] {}: {}\n", i + 1, info.inputTitles[i],
                                     info.inputValues[i].empty() ? "_" : info.inputValues[i]);
        }
    }
    std::cout << "\n Buttons:";
    for (const auto& btn : info.buttons) {
        std::cout << " [" << btn << "]";
    }
    if (std::ranges::none_of(info.buttonConsumes, [](bool c) { return c; })) {
        std::cout << " [" << info.consumeButtonText << "]";
    }
    std::cout << "\n";
    std::cout << std::format("{:=<50}\n", "");
    std::cout << "Action> " << std::flush;
}

void presentJson(const PopupInfo& info)
{
    using json = nlohmann::json;
    json j;
    j["type"]    = "popup";
    j["id"]      = std::format("popup_UUT{}_{}", info.uut, info.name);
    j["uut"]     = info.uut;
    j["name"]    = info.name;
    j["texts"]   = info.texts;

    json inputs = json::array();
    for (size_t i = 0; i < info.inputTitles.size(); ++i) {
        inputs.push_back({{"index", i + 1}, {"title", info.inputTitles[i]}, {"value", info.inputValues[i]}});
    }
    j["inputs"] = inputs;

    json buttons = json::array();
    for (const auto& btn : info.buttons) {
        buttons.push_back(btn);
    }
    if (std::ranges::none_of(info.buttonConsumes, [](bool c) { return c; })) {
        buttons.push_back(info.consumeButtonText);
    }
    j["buttons"] = buttons;

    std::cout << j.dump() << "\n" << std::flush;
}

struct ParsedResponse {
    std::string              button;
    std::map<int, std::string> inputs;    // 1-indexed
};

ParsedResponse parseHumanLine(const std::string& line)
{
    ParsedResponse resp;
    // Check if it's an input assignment: <number>=<value>
    if (auto eqPos = line.find('='); eqPos != std::string::npos) {
        std::string indexStr = line.substr(0, eqPos);
        try {
            int idx        = std::stoi(indexStr);
            resp.inputs[idx] = line.substr(eqPos + 1);
            return resp;
        }
        catch (...) {
            // Not a number= pattern, treat as button
        }
    }
    resp.button = line;
    return resp;
}

ParsedResponse parseJsonLine(const std::string& line)
{
    using json = nlohmann::json;
    ParsedResponse resp;
    try {
        auto j = json::parse(line);
        if (j.contains("button")) { resp.button = j["button"].get<std::string>(); }
        if (j.contains("inputs")) {
            for (auto& [key, value] : j["inputs"].items()) {
                resp.inputs[std::stoi(key)] = value.get<std::string>();
            }
        }
    }
    catch (const std::exception& e) {
        BR_LOG_ERROR(s_tag, "Failed to parse JSON response: {}", e.what());
    }
    return resp;
}

/// Find which button matches (case-insensitive)
int findButton(const std::string& name, const PopupInfo& info)
{
    auto lower = [](std::string s) {
        std::ranges::transform(s, s.begin(), ::tolower);
        return s;
    };
    std::string lowerName = lower(name);
    for (size_t i = 0; i < info.buttons.size(); ++i) {
        if (lower(info.buttons[i]) == lowerName) { return static_cast<int>(i); }
    }
    if (lower(info.consumeButtonText) == lowerName) { return -1; }    // Cancel/consume button
    return -2;    // Not found
}

}    // namespace

void importHeadlessPopup(sol::state_view    lua,
                         std::size_t        uut,
                         const std::string& outputFormat,
                         int                timeoutSeconds,
                         std::mutex&        ioMutex)
{
    lua.script_file("lua/core/sdk/popup.lua");
    lua["__popup"] = lua.create_table();

    // Consume is a no-op in headless (we handle consumption inline in Show)
    lua["__popup"]["Consume"] = [](sol::table) {};

    if (lua["Context"]["info"]["stage"].get<int>() == 3) {    // Stage::execution
        lua["__popup"]["Show"] = [&ioMutex, uut, outputFormat, timeoutSeconds](sol::table builder) -> std::vector<std::string> {
            auto info = PopupInfo::extract(uut, builder);

            // Lock for the entire popup interaction — serializes popups across UUTs
            std::lock_guard lock(ioMutex);

            // Present the popup
            if (outputFormat == "json") {
                presentJson(info);
            }
            else {
                presentHuman(info);
            }

            // Read response
            auto startTime = std::chrono::steady_clock::now();
            bool consumed  = false;

            while (!consumed) {
                // Check timeout
                if (timeoutSeconds > 0) {
                    auto elapsed = std::chrono::steady_clock::now() - startTime;
                    if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= timeoutSeconds) {
                        if (outputFormat == "json") {
                            nlohmann::json timeout;
                            timeout["type"]            = "popup_timeout";
                            timeout["id"]              = std::format("popup_UUT{}_{}", uut, info.name);
                            timeout["timeout_seconds"] = timeoutSeconds;
                            std::cout << timeout.dump() << "\n" << std::flush;
                        }
                        else {
                            std::cout << std::format("  [TIMEOUT] Popup timed out after {}s, auto-cancelled.\n",
                                                    timeoutSeconds);
                        }
                        return info.inputValues;
                    }
                }

                std::string line;
                if (!std::getline(std::cin, line)) {
                    // EOF — auto-cancel
                    BR_LOG_WARN(s_tag, "stdin EOF while waiting for popup response, auto-cancelling");
                    return info.inputValues;
                }
                // Trim trailing whitespace (handles \r from piped input on Windows)
                while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
                    line.pop_back();
                }

                if (line.empty()) { continue; }

                // Parse based on format
                ParsedResponse resp;
                if (outputFormat == "json") {
                    resp = parseJsonLine(line);
                    // In JSON mode, a single response sets all inputs and a button
                    for (auto& [idx, value] : resp.inputs) {
                        if (idx >= 1 && idx <= static_cast<int>(info.inputValues.size())) {
                            info.inputValues[idx - 1] = value;
                        }
                    }
                    if (!resp.button.empty()) {
                        int btnIdx = findButton(resp.button, info);
                        if (btnIdx >= 0 && btnIdx < static_cast<int>(info.buttonActions.size())) {
                            auto result = info.buttonActions[btnIdx](info.inputValues);
                            if (!result.valid()) {
                                sol::error err = result;
                                BR_LOG_ERROR(s_tag, "Button action error: {}", err.what());
                            }
                        }
                        consumed = true;
                    }
                }
                else {
                    resp = parseHumanLine(line);
                    if (!resp.inputs.empty()) {
                        // Set input values
                        for (auto& [idx, value] : resp.inputs) {
                            if (idx >= 1 && idx <= static_cast<int>(info.inputValues.size())) {
                                info.inputValues[idx - 1] = value;
                                std::cout << std::format("  [OK] Input {} ({}) = \"{}\"\n", idx,
                                                        info.inputTitles[idx - 1], value);
                                std::cout << "Action> " << std::flush;
                            }
                            else {
                                std::cout << std::format("  [ERR] Invalid input index: {}\n", idx);
                                std::cout << "Action> " << std::flush;
                            }
                        }
                    }
                    else if (line == "?") {
                        // Re-display
                        presentHuman(info);
                    }
                    else if (!resp.button.empty()) {
                        int btnIdx = findButton(resp.button, info);
                        if (btnIdx == -2) {
                            std::cout << std::format("  [ERR] Unknown button: \"{}\"\n", resp.button);
                            std::cout << "Action> " << std::flush;
                        }
                        else {
                            // Call the button's action with the current input values
                            if (btnIdx >= 0 && btnIdx < static_cast<int>(info.buttonActions.size())) {
                                auto result = info.buttonActions[btnIdx](info.inputValues);
                                if (!result.valid()) {
                                    sol::error err = result;
                                    BR_LOG_ERROR(s_tag, "Button action error: {}", err.what());
                                }
                            }
                            consumed = true;
                        }
                    }
                }
            }

            {
                if (outputFormat == "json") {
                    // No additional output needed
                }
                else {
                    std::cout << "  -> Popup consumed.\n" << std::flush;
                }
            }

            return info.inputValues;
        };
    }
    else {
        // Generation/Validation — same as GUI: return empty inputs immediately
        lua["__popup"]["Show"] = [uut](sol::table builder) -> std::vector<std::string> {
            auto info = PopupInfo::extract(uut, builder);
            return info.inputValues;
        };
    }
}

}    // namespace Frasy::Headless
