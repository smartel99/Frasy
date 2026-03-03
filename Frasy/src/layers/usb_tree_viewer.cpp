/**
 * @file    usb_tree_viewer.cpp
 * @author  Sam Martel
 * @date    2026-03-03
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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "usb_tree_viewer.h"

#include "utils/misc/visit.h"
#include "utils/usb_enumerator/usb_enumerator.h"

#include <imgui.h>

namespace Frasy {

namespace {
void RenderNodes(const std::vector<Usb::Node>& tree);

struct RenderRootHubInfo {
    void operator()(const Usb::RootHubInfo& info) const
    {
        bool hovered = false;
        if (ImGui::TreeNodeEx(&info, ImGuiTreeNodeFlags_DefaultOpen, "Root Hub: %s", info.hubName.c_str())) {
            hovered = ImGui::IsItemHovered();
            ImGui::BulletText("%d Nodes", info.nodes.size());
            RenderNodes(info.nodes);

            ImGui::TreePop();
        }
        else {
            hovered = ImGui::IsItemHovered();
        }

        if (hovered) {
            ImGui::BeginTooltip();
            ImGui::Text("Hub Name: %s", info.leafName.c_str());
            ImGui::EndTooltip();
        }
    }
};

struct RenderHostControllerInfo {
    void operator()(const Usb::HostControllerInfo& info) const
    {
        bool hovered = false;
        if (ImGui::TreeNodeEx(&info,
                              ImGuiTreeNodeFlags_DefaultOpen,
                              "Host Controller: %s - %s",
                              info.deviceDesc.c_str(),
                              info.driverKey.c_str())) {
            hovered = ImGui::IsItemHovered();
            RenderRootHubInfo {}(info.rootHub);
            ImGui::TreePop();
        }
        else {
            hovered = ImGui::IsItemHovered();
        }
        if (hovered) {
            ImGui::BeginTooltip();
            ImGui::Text("VendorID: %d, DeviceID: %d, SubSysId: %d, Revision: %d",
                        info.vendorId,
                        info.deviceId,
                        info.subSysId,
                        info.revision);
            if (!info.busDeviceFunctionValid) { ImGui::Text("Invalid bus/device/function"); }
            else {
                ImGui::Text(
                  "Bus number: %d, device: %d, function: %d", info.busNumber, info.busDevice, info.busFunction);
            }
            ImGui::EndTooltip();
        }
    }
};

void RenderStringDescriptors(const Usb::UsbNodeConnectionInformationEx&    connInfo,
                             const std::vector<Usb::StringDescriptorNode>& descriptors)
{
    ImGui::BulletText("# of supported configurations: %d", connInfo.DeviceDescriptor.bNumConfigurations);
    ImGui::BulletText("# of endpoints: %d", connInfo.NumberOfOpenPipes);

    auto renderDesc = [&](std::string_view name, uint8_t idx) {
        std::string_view str = "Absent";
        if (idx != 0 && idx < descriptors.size()) { str = descriptors[idx].stringDescriptor.toUtf8(); }
        ImGui::BulletText("%s: %s", name.data(), str.data());
    };
    renderDesc("Manufacturer", connInfo.DeviceDescriptor.iManufacturer);
    renderDesc("Product", connInfo.DeviceDescriptor.iProduct);
    renderDesc("Serial Number", connInfo.DeviceDescriptor.iSerialNumber);
    renderDesc("Device Class", connInfo.DeviceDescriptor.bDeviceClass);
    // The first descriptor is the language ID descriptors.
    for (auto& descriptor : descriptors | std::views::drop(1)) {
        ImGui::BulletText("Descriptor Index: %d, language ID: %d, descType: %d, data: %s",
                          descriptor.descriptorIndex,
                          descriptor.languageId,
                          descriptor.stringDescriptor.descriptorType,
                          descriptor.stringDescriptor.toUtf8().c_str());
    }
}

struct RenderExternalHubInfo {
    void operator()(const Usb::ExternalHubInfo& info) const
    {
        bool hovered = false;
        if (ImGui::TreeNodeEx(&info, ImGuiTreeNodeFlags_DefaultOpen, "%s", info.leafName.c_str())) {
            hovered = ImGui::IsItemHovered();
            ImGui::BulletText("%d Nodes", info.nodes.size());
            RenderNodes(info.nodes);

            ImGui::TreePop();
        }
        else {
            hovered = ImGui::IsItemHovered();
        }

        if (hovered) {
            ImGui::BeginTooltip();
            ImGui::Text("Hub Name: %s", info.hubName.c_str());
            RenderStringDescriptors(info.connectionInfo, info.stringDescriptors);
            ImGui::EndTooltip();
        }
    }
};

struct RenderDeviceInfo {
    void operator()(const Usb::DeviceInfo& info) const
    {
        ImGui::BulletText("%s", info.leafName.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            RenderStringDescriptors(info.connectionInfo, info.stringDescriptors);
            ImGui::EndTooltip();
        }
    }
};

void RenderNodes(const std::vector<Usb::Node>& tree)
{
    for (const auto& node : tree) {
        visit(node, RenderHostControllerInfo {}, RenderRootHubInfo {}, RenderExternalHubInfo {}, RenderDeviceInfo {});
    }
}
}    // namespace

UsbTreeViewer::UsbTreeViewer() : Layer("UsbTreeViewer")
{
}

void UsbTreeViewer::onImGuiRender()
{
    if (!m_isVisible) { return; }

    if (ImGui::Begin("USB Tree", &m_isVisible)) {
        bool isRefreshing = m_isRefreshing;    // To avoid TOCTOU :D
        ImGui::Text("%d nodes found %s", m_nodes.size(), isRefreshing ? "(Refreshing...)" : "");
        if (ImGui::Button("Refresh")) {
            m_refreshFuture = std::async(std::launch::async, [this] {
                m_isRefreshing = true;
                m_nodes        = Usb::EnumerateUsbTree();
                m_isRefreshing = false;
            });
        }
        if (!isRefreshing) { RenderNodes(m_nodes); }
    }
    ImGui::End();
}
}    // namespace Frasy
