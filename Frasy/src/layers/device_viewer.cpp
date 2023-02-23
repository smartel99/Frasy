/**
 * @file    device_viewer.cpp
 * @author  Samuel Martel
 * @date    2022-12-20
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
#include "device_viewer.h"

#include "utils/communication/serial/device_map.h"
#include "utils/communication/serial/enumerator.h"

#include <thread>

namespace Frasy
{
DeviceViewer::DeviceViewer() noexcept : m_deviceMap(Communication::DeviceMap::Get())
{
    m_deviceMap.ScanForDevices();
}

void DeviceViewer::OnImGuiRender()
{
    if (!m_isVisible) { return; }

    if (ImGui::Begin(s_windowName, &m_isVisible))
    {
        if (ImGui::Button("Rescan"))
        {
            if (!m_deviceMap.IsScanning()) { m_deviceMap.ScanForDevices(); }
            else { BR_APP_WARN("Scan already in progress!"); }
        }

        ImGui::Separator();

        if (!m_deviceMap.IsScanning()) { RenderDeviceList(); }
    }
    ImGui::End();
}

void DeviceViewer::SetVisibility(bool visibility)
{
    m_isVisible = true;
    ImGui::SetWindowFocus(s_windowName);
}

void DeviceViewer::RenderDeviceList()
{
    for (auto&& [id, device] : m_deviceMap)
    {
        std::string port  = device.GetPort();
        std::string label = fmt::format("{} - {}", id, port);

        bool isOpen = device.IsOpen();

        // If device is open, display it as a nice lil' green.
        auto TreeNodeOpen = [isOpen, &label]() -> bool
        {
            if (isOpen) { ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0AB002); }
            bool open = ImGui::TreeNode(label.c_str());
            if (isOpen) { ImGui::PopStyleColor(); }
            return open;
        };

        if (TreeNodeOpen())
        {
            const Frasy::Actions::Identify::Info& devInfo = device.GetInfo();
            ImGui::PushID(devInfo.Uuid.c_str());
            if (isOpen)
            {
                const auto& col = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
                ImGui::PushStyleColor(ImGuiCol_Button, col);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
            }
            if (ImGui::Button("Open") && !isOpen) { device.Open(); }
            if (isOpen) { ImGui::PopStyleColor(3); }

            if (ImGui::TreeNode("Information"))
            {
                ImGui::BulletText("ID: %d", id);
                ImGui::BulletText("Port: %s", port.c_str());
                ImGui::BulletText("UUID: %s", devInfo.Uuid.c_str());
                ImGui::BulletText("Loaded Firmware: %s - %s", devInfo.PrjName.c_str(), devInfo.Version.c_str());
                ImGui::BulletText("Built: %s", devInfo.Built.c_str());

                if (ImGui::TreeNode("Features"))
                {
                    auto commands = device.GetCommands();
                    if (commands.empty()) { ImGui::Text("No commands supported"); }
                    else
                    {
                        ImGui::Text("%zu supported commands", commands.size());
                        for (const auto& command : commands) { ImGui::BulletText("%s", command.Name.c_str()); }
                    }
                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Options"))
            {
                bool shouldLog = device.GetLog();
                if (ImGui::Checkbox("Log", &shouldLog)) { device.SetLog(shouldLog); }
                if (ImGui::Button("Reset")) { device.Reset(); }
                if (ImGui::Button("Update")) { BR_APP_WARN("Instrumentation card update not implemented"); }
                ImGui::TreePop();
            }
            ImGui::PopID();

            ImGui::TreePop();
        }

        ImGui::Separator();
    }
}
}    // namespace Frasy
