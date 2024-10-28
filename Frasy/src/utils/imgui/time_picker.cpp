/**
 * @file    time_picker.cpp
 * @author  Samuel Martel
 * @date    2024-10-22
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */
#include "time_picker.h"
#include "spin_input.h"

#include <imgui.h>

#include <Brigerad/Core/Log.h>

#include <string>
#include <unordered_map>

namespace Frasy::Widget {
namespace {
struct Time {
    Time(const std::chrono::seconds& seconds)
    {
        std::chrono::hh_mm_ss hhmmss {seconds};
        hour   = hhmmss.hours().count();
        minute = hhmmss.minutes().count();
        second = hhmmss.seconds().count();
    }
    int hour   = 0;
    int minute = 0;
    int second = 0;
};
// To hold the time while it is being set.
std::unordered_map<std::string, Time> s_activeTimePickers = {};
}    // namespace

bool timePicker(const std::string& title, bool* isOpen, std::chrono::seconds& seconds)
{
    ImVec2 windowSize = ImGui::GetMainViewport()->WorkSize;
    ImGui::SetNextWindowPos(ImVec2(windowSize.x / 2.0f, windowSize.y / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(570.0f, 315.0f), ImGuiCond_Always);
    if (!ImGui::BeginPopupModal(title.c_str(), isOpen, ImGuiWindowFlags_NoDecoration)) { return false; }
    ImGui::PushID(title.c_str());

    ImGuiIO& io = ImGui::GetIO();
    ImGui::PushFont(io.Fonts->Fonts[1]);

    auto [it, _] = s_activeTimePickers.try_emplace(title, seconds);
    auto& time   = it->second;

    ImGui::BeginVertical("", ImGui::GetContentRegionAvail(), 0.5f);
    ImGui::Spring(0.1f);

    ImGui::Text(title.c_str());

    ImGui::Spring(0.1f);

    ImGui::BeginHorizontal("Time", ImVec2(0, 0), 0.5f);
    ImGui::Spring();
    spinInt("hours", &time.hour, 1, 5, 0, 99, "%02d");
    spinInt("minutes", &time.minute, 1, 5, 0, 59, "%02d");
    spinInt("seconds", &time.second, 1, 5, 0, 59, "%02d");
    ImGui::Spring();
    ImGui::EndHorizontal();

    ImGui::Spring(0.5f);
    ImGui::TextDisabled("Press and hold the control key for bigger steps");

    ImGui::Spring(0.3f);

    ImGui::BeginHorizontal("Buttons", ImVec2(0.0f, ImGui::GetFrameHeightWithSpacing()), 0.5f);
    ImGui::Spring(0.5f);
    bool hasChanged = false;
    ImGui::PushStyleColor(ImGuiCol_Button, 0x664242FA);           // Red
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF4242FA);    // Red but stronger
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xFF0F0FFA);     // Red but stronger, but less strong
    if (ImGui::Button("Cancel")) { hasChanged = true; }
    ImGui::PopStyleColor(3);

    ImGui::Spring(1.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, 0x6643FA43);           // Green
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0xFF43FA43);    // Green but stronger
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0xB210FA10);     // Green but stronger, but less strong
    if (ImGui::Button("  Ok  ")) {
        hasChanged = true;
        seconds    = std::chrono::seconds(time.hour * 3600 + time.minute * 60 + time.second);
    }
    ImGui::PopStyleColor(3);
    ImGui::Spring(0.5f);
    ImGui::EndHorizontal();

    ImGui::Spring(0.1f);
    ImGui::EndVertical();

    ImGui::PopFont();

    ImGui::PopID();
    ImGui::EndPopup();

    if (hasChanged) {
        s_activeTimePickers.erase(title);
        *isOpen = false;
    }

    return hasChanged;
}
}    // namespace Frasy::Widget
