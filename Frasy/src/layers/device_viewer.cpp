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

namespace Frasy {
DeviceViewer::DeviceViewer() noexcept : m_deviceMap(Communication::DeviceMap::Get())
{
    m_deviceMap.scanForDevices();
}

void DeviceViewer::onImGuiRender()
{
    if (!m_isVisible) { return; }

    if (ImGui::Begin(s_windowName, &m_isVisible, ImGuiWindowFlags_NoDocking)) {
        if (ImGui::Button("Rescan")) {
            if (!m_deviceMap.isScanning()) { m_deviceMap.scanForDevices(); }
            else {
                BR_APP_WARN("Scan already in progress!");
            }
        }

        ImGui::Separator();

        if (!m_deviceMap.isScanning()) { renderDeviceList(); }
    }
    ImGui::End();
}

void DeviceViewer::setVisibility(bool visibility)
{
    m_isVisible = true;
    ImGui::SetWindowFocus(s_windowName);
}

void DeviceViewer::renderDeviceList()
{
    for (auto&& [id, device] : m_deviceMap) {
        std::string port  = device.getPort();
        std::string label = fmt::format("{} - {}", id, port);

        bool isOpen = device.isOpen();

        // If device is open, display it as a nice lil' green.
        auto TreeNodeOpen = [isOpen, &label]() -> bool {
            if (isOpen) { ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0AB002); }
            bool open = ImGui::TreeNode(label.c_str());
            if (isOpen) { ImGui::PopStyleColor(); }
            return open;
        };

        if (TreeNodeOpen()) {
            const Frasy::Actions::Identify::Info& devInfo = device.getInfo();
            ImGui::PushID(devInfo.Uuid.c_str());
            if (isOpen) {
                const auto& col = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
                ImGui::PushStyleColor(ImGuiCol_Button, col);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, col);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);
            }
            if (ImGui::Button("open") && !isOpen) { device.open(); }
            if (isOpen) { ImGui::PopStyleColor(3); }

            if (ImGui::TreeNode("Information")) {
                ImGui::BulletText("ID: %d", id);
                ImGui::BulletText("Port: %s", port.c_str());
                ImGui::BulletText("UUID: %s", devInfo.Uuid.c_str());
                ImGui::BulletText("Loaded Firmware: %s - %s", devInfo.PrjName.c_str(), devInfo.Version.c_str());
                ImGui::BulletText("Built: %s", devInfo.Built.c_str());

                if (ImGui::TreeNode("Features")) {
                    if (ImGui::TreeNode("Commands")) {
                        renderDeviceCommands(device.getCommands(), device.GetTypeManager());
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("Enums")) {
                        renderDeviceEnums(device.getEnums());
                        ImGui::TreePop();
                    }

                    if (ImGui::TreeNode("Structures")) {
                        renderDeviceStructs(device.getStructs(), device.GetTypeManager());
                        ImGui::TreePop();
                    }

                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Options")) {
                bool shouldLog = device.getLog();
                if (ImGui::Checkbox("Log", &shouldLog)) { device.setLog(shouldLog); }
                if (ImGui::Button("reset")) { device.reset(); }
                if (ImGui::Button("Update")) { BR_APP_WARN("Instrumentation card update not implemented"); }
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Pending Transactions")) {
                for (auto&& transId : device.getPendingTransactions()) {
                    ImGui::BulletText("%08X", transId);
                }
                ImGui::TreePop();
            }
            ImGui::PopID();

            ImGui::TreePop();
        }

        ImGui::Separator();
    }
}

void DeviceViewer::renderDeviceCommands(
  const std::unordered_map<Actions::cmd_id_t, Actions::CommandInfo::Reply>& commands, const Type::Manager& manager)
{
    if (commands.empty()) { ImGui::Text("No commands supported"); }
    else {
        ImGui::Text("%zu supported commands", commands.size());
        for (const auto& [_, command] : commands) {
            if (ImGui::TreeNode(command.Name.c_str())) {
                ImGui::BulletText("ID: %d", command.Id);
                if (!command.Alias.empty()) { ImGui::BulletText("Alias: %s", command.Alias.c_str()); }
                ImGui::Bullet();
                ImGui::SameLine();
                ImGui::TextWrapped("Help: %s", command.Help.c_str());

                if (command.Parameters.empty()) { ImGui::BulletText("No Parameters"); }
                else {
                    renderCommandValues("Parameters", command.Parameters, manager);
                }
                if (command.Returns.empty()) { ImGui::BulletText("No Returned Values"); }
                else {
                    renderCommandValues("Returned", command.Returns, manager);
                }

                ImGui::TreePop();
            }
        }
    }
}

void DeviceViewer::renderDeviceEnums(const std::unordered_map<Actions::cmd_id_t, Type::Enum>& enums)
{
    if (enums.empty()) { ImGui::Text("No enums defined"); }
    else {
        ImGui::Text("%zu defined enums", enums.size());
        for (const auto& [id, e] : enums) {
            if (ImGui::TreeNode(e.Name.c_str())) {
                ImGui::BulletText("ID: %d", static_cast<int>(id));
                ImGui::BulletText("Size of each fields: %d bytes", e.UnderlyingSize);
                ImGui::Bullet();
                ImGui::SameLine();
                ImGui::TextWrapped("Description: %s",
                                   e.Description.empty() ? "<none provided>" : e.Description.c_str());

                if (e.Fields.empty()) { ImGui::BulletText("No fields"); }
                else {
                    std::string fieldsLabel = std::format("Fields ({} fields defined)", e.Fields.size());
                    if (ImGui::TreeNode(fieldsLabel.c_str())) {
                        for (const auto& field : e.Fields) {
                            ImGui::BulletText("%s: %d", field.Name.c_str(), field.Value);
                            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", field.Description.c_str()); }
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
    }
}

void DeviceViewer::renderDeviceStructs(const std::unordered_map<Actions::cmd_id_t, Type::Struct>& structs,
                                       const Type::Manager&                                       manager)
{
    if (structs.empty()) { ImGui::Text("No structs defined"); }
    else {
        ImGui::Text("%zu defined structs", structs.size());
        for (const auto& [id, s] : structs) {
            if (ImGui::TreeNode(s.Name.c_str())) {
                ImGui::BulletText("ID: %d", static_cast<int>(id));

                ImGui::Bullet();
                ImGui::SameLine();
                ImGui::TextWrapped("Description: %s",
                                   s.Description.empty() ? "<none provided>" : s.Description.c_str());

                if (s.Fields.empty()) { ImGui::BulletText("No fields"); }
                else {
                    std::string fieldsLabel = std::format("Fields ({} fields defined)", s.Fields.size());
                    if (ImGui::TreeNode(fieldsLabel.c_str())) {
                        for (const auto& field : s.Fields) {
                            std::string fieldType    = manager.GetTypeName(field.Type);
                            std::string fieldTypeStr = fieldType;
                            if (field.Count == 0) { fieldTypeStr = std::format("Vector of {}", fieldType); }
                            else if (field.Count != 1) {
                                fieldTypeStr = std::format("{}[{}]", fieldType, field.Count);
                            }
                            ImGui::BulletText("%s: %s", field.Name.c_str(), fieldTypeStr.c_str());
                            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", field.Description.c_str()); }
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
    }
}

void DeviceViewer::renderCommandValues(std::string_view                   name,
                                       const std::vector<Actions::Value>& values,
                                       const Type::Manager&               manager)
{
    if (ImGui::TreeNode(name.data())) {
        for (auto&& value : values) {
            if (ImGui::TreeNode(value.Name.c_str())) {
                std::string typeName = manager.GetTypeName(value.Type);
                std::string type     = typeName;
                if (value.Count == 0) { type = std::format("vector of {}", typeName); }
                else if (value.Count != 1) {
                    type = std::format("{}[{}]", typeName, value.Count);
                }
                ImGui::BulletText("Type: %s", type.c_str());
                if (!value.Min.empty()) { ImGui::BulletText("Min: %s", value.Min.c_str()); }
                if (!value.Max.empty()) { ImGui::BulletText("Max: %s", value.Max.c_str()); }
                ImGui::Bullet();
                ImGui::SameLine();
                ImGui::TextWrapped("Help: %s", value.Help.empty() ? "<unavailable>" : value.Help.c_str());

                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
}

}    // namespace Frasy
