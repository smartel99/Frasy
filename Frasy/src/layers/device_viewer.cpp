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

#include "frasy_interpreter.h"

#include <algorithm>
#include <charconv>
#include <regex>
#include <string>
#include <vector>

namespace Frasy {

void to_json(nlohmann::json& j, const DeviceViewer::DeviceViewerOptions::WhitelistItem& item)
{
    j = nlohmann::json {{"vid", item.vid}, {"pid", item.pid}};
    if (item.rev.has_value()) { j["rev"] = *item.rev; }
    if (item.mi.has_value()) { j["mi"] = *item.mi; }
}
void from_json(const nlohmann::json& j, DeviceViewer::DeviceViewerOptions::WhitelistItem& item)
{
    if (j.contains("vid")) { j["vid"].get_to(item.vid); }
    if (j.contains("pid")) { j["pid"].get_to(item.pid); }
    if (j.contains("rev")) { item.rev = j["rev"].get<int>(); }
    if (j.contains("mi")) { item.mi = j["mi"].get<int>(); }
}

void to_json(nlohmann::json& j, const DeviceViewer::DeviceViewerOptions& options)
{
    j = nlohmann::json {{"lastDevice", options.lastDevice}, {"usbWhitelist", options.usbWhitelist}};
}
void from_json(const nlohmann::json& j, DeviceViewer::DeviceViewerOptions& options)
{
    if (j.contains("lastDevice")) { j["lastDevice"].get_to(options.lastDevice); }
    if (j.contains("usbWhitelist")) { j["usbWhitelist"].get_to(options.usbWhitelist); }
}

DeviceViewer::DeviceViewer(SlCan::Device& device) noexcept : m_device(device)
{
    m_device.m_monitorFunc = [this](const SlCan::Packet& pkt) {
        m_pktCount++;

        // To make sure we don't use infinite ram at one point.
        // TODO remove this!!!
        while (m_device.m_queue.size() >= 100'000) {
            m_device.m_queue.pop();
        }

        m_packetsRxInCurrentSecond++;
        m_bytesRxInCurrentSecond += pkt.sizeOfSerialPacket();
        if (!m_isVisible) { return; }
        if (!SlCan::commandIsTransmit(pkt.command)) { return; }

        m_networkState[pkt.data.packetData.id] = pkt.data.packetData;
    };

    m_resetter = std::jthread([this](std::stop_token stopToken) {
        using namespace std::chrono_literals;

        while (!stopToken.stop_requested()) {
            m_packetsRxInLastSecond    = (m_packetsRxInLastSecond + m_packetsRxInCurrentSecond) / 2;
            m_packetsRxInCurrentSecond = 0;

            m_kilobytesRxInLastSecond =
              (m_kilobytesRxInLastSecond + static_cast<float>(m_bytesRxInCurrentSecond) / 1024.0f) / 2.0f;
            m_bytesRxInCurrentSecond = 0;
            std::this_thread::sleep_for(1s);
        }
    });
}

bool DeviceViewer::DeviceViewerOptions::WhitelistItem::operator==(const std::string& rhs) const
{
    WhitelistItem other {};
    std::regex    search("(VID_|PID_|REV_|MI_)([a-f]|[A-F]|[0-9])\\w+");

    auto matchBegin = std::sregex_iterator(rhs.begin(), rhs.end(), search);
    auto matchEnd   = std::sregex_iterator();

    for (std::sregex_iterator i = matchBegin; i != matchEnd; ++i) {
        std::smatch match     = *i;
        std::string match_str = match.str();

        size_t delim = match_str.find('_');
        auto   word  = match_str.substr(0, delim);
        auto   num   = match_str.substr(delim + 1);

        int val = {};
        std::from_chars(num.data(), num.data() + num.size(), val, 16);

        if (word == "VID") { other.vid = val; }
        else if (word == "PID") {
            other.pid = val;
        }
        else if (word == "REV") {
            other.rev = val;
        }
        else if (word == "MI") {
            other.mi = val;
        }
    }

    return *this == other;
}

void DeviceViewer::onAttach()
{
    m_options = FrasyInterpreter::Get().getConfig().getField<DeviceViewerOptions>("communication");

    // Load the last device from the config, try to open it.
    if (!m_options.lastDevice.empty()) { m_device = SlCan::Device(m_options.lastDevice); }
    // If fails: get a list of the connected devices, search for a whitelisted VID/PID pair.
    // If found, open the first one found.
    // Otherwise, do nothing, display combo box with the ports and let the user choose.
    if (!m_device.isOpen()) {
        m_ports = serial::list_ports();
        auto it = std::find_if(
          m_ports.begin(),
          m_ports.end(),
          [&usbWhitelist = m_options.usbWhitelist](const serial::PortInfo& info) -> bool {
              return std::any_of(usbWhitelist.begin(),
                                 usbWhitelist.end(),
                                 [&vidPid = info.hardware_id](const DeviceViewerOptions::WhitelistItem& item) -> bool {
                                     return item == vidPid;
                                 });
          });

        if (it != m_ports.end()) {
            BR_APP_INFO("Found whitelisted device on port {} ({})!", it->port, it->hardware_id);
            m_device = SlCan::Device(it->port);
            if (m_device.isOpen()) { m_options.lastDevice = it->port; }
        }
    }
}

void DeviceViewer::onDetach()
{
    // Save the currently opened device, if one exists.
    FrasyInterpreter::Get().getConfig().setField("communication", m_options);
}

void DeviceViewer::onImGuiRender()
{
    if (ImGui::BeginMainMenuBar()) {
        ImGui::BeginHorizontal("menuBarSpan", ImVec2 {ImGui::GetContentRegionAvail().x, 0.0f});
        ImGui::Spring(1);
        ImGui::Separator();

        static constexpr ImVec4 s_connectedColor    = {40, 255, 0, 255};
        static constexpr ImVec4 s_disconnectedColor = {255, 0, 0, 255};
        ImVec4                  color               = m_device.isOpen() ? s_connectedColor : s_disconnectedColor;
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        if (m_device.isOpen()) { ImGui::Text("Connected (%s)  ", m_device.getPort().c_str()); }
        else {
            ImGui::Text("Disconnected  ");
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemClicked()) { setVisibility(true); }

        auto renderNetworkUsage = [](float usage) {
            auto progressColor = ImVec4 {1.0f, 1.0f, 0, 1.0f};

            // Under 50% usage, remove red to make it greener.
            // Above 50% usage, remove green to make it more red.
            if (usage <= 0.5f) { progressColor.x -= (0.5f - usage) * 2.0f; }
            else {
                // Will be in range (0.5, 1.0], bring into [0.0, 0.5] range.
                progressColor.y -= (usage - 0.5f) * 2.0f;
            }

            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, progressColor);
            ImGui::ProgressBar(usage, ImVec2 {50.0f, 0.0f}, "");
            ImGui::PopStyleColor();
        };

        // Combine both Rx and Tx into one bar, then expand in the pop up.
        renderNetworkUsage((m_kilobytesRxInLastSecond + m_kilobytesTxInLastSecond) / 300.0f);

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Addresses: %zu", m_networkState.size());
            ImGui::Text("Packets Pending: %zu", m_device.m_queue.size());
            ImGui::Text("Packets Received: %zu", m_pktCount);
            ImGui::Text("Rx: %zu pkt/s (%0.3f kB/s)", m_packetsRxInLastSecond, m_kilobytesRxInLastSecond);
            ImGui::SameLine();
            renderNetworkUsage(m_kilobytesRxInLastSecond / 150.0f);
            ImGui::Text("Tx: %zu pkt/s (%0.3f kB/s)", m_packetsTxInLastSecond, m_kilobytesTxInLastSecond);
            ImGui::SameLine();
            renderNetworkUsage(m_kilobytesTxInLastSecond / 150.0f);
            ImGui::EndTooltip();
        }
        ImGui::EndHorizontal();
        ImGui::EndMainMenuBar();
    }

    if (!m_isVisible) { return; }

    if (ImGui::Begin(s_windowName, &m_isVisible, ImGuiWindowFlags_NoDocking)) {

        if (m_device.isOpen()) {
            ImGui::Text("Connected to: %s", m_device.getPort().c_str());
            ImGui::SameLine();
            if (ImGui::Button("Close")) { m_device.close(); }
        }
        else {
            if (ImGui::Button("Open")) { m_device.open(); }
        }
        ImGui::Separator();
        ImGui::Text("Network State: %zu addresses, %zu pending packets (%zu total), %zu pkt/s, %0.3f kB/s",
                    m_networkState.size(),
                    m_device.m_queue.size(),
                    m_pktCount,
                    m_packetsRxInLastSecond,
                    m_kilobytesRxInLastSecond);
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            m_networkState.clear();
            m_pktCount = 0;
        }
        renderNetworkState();
    }
    ImGui::End();
}

void DeviceViewer::setVisibility(bool visibility)
{
    m_isVisible = visibility;
    ImGui::SetWindowFocus(s_windowName);
}

void DeviceViewer::refreshPorts()
{
    m_ports = serial::list_ports();
}

void DeviceViewer::renderNetworkState()
{
    BR_PROFILE_FUNCTION();

    static ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_Sortable |
                                        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SortMulti |
                                        ImGuiTableFlags_SortTristate;

    float maxY = ImGui::GetContentRegionAvail().y;
    if (ImGui::BeginTable("entries", 5, tableFlags, ImVec2 {0.0f, maxY})) {
        ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_DefaultSort);
        ImGui::TableSetupColumn("IsExt");
        ImGui::TableSetupColumn("IsRTR");
        ImGui::TableSetupColumn("DLC");
        ImGui::TableSetupColumn("Data");
        ImGui::TableHeadersRow();
        for (const auto& [id, packet] : m_networkState) {
            ImGui::TableNextRow();
            renderNetworkPacket(packet);
        }
        ImGui::EndTable();
    }
}

void DeviceViewer::renderNetworkPacket(const SlCan::CanPacket& packet)
{
    BR_PROFILE_FUNCTION();

    static auto renderColumn = [](size_t columnId, const char* fmt, const auto& data) {
        if (ImGui::TableSetColumnIndex(static_cast<int>(columnId))) {
            float wrapPos = ImGui::GetContentRegionMax().x;
            ImGui::PushTextWrapPos(wrapPos);
            ImGui::TextWrapped(fmt, data);
            ImGui::PopTextWrapPos();
        }
    };
    auto formatData = [](const uint8_t* data, size_t len) -> std::string {
        return fmt::format("{:#02x}", fmt::join(std::span {data, len}, ", "));
    };

    ImGui::BeginGroup();
    // Use the pointer location as a unique ID.
    ImGui::PushID(static_cast<int>(packet.id));

    renderColumn(0, "%#08x", packet.id);
    renderColumn(1, "%c", packet.isExtended ? 'Y' : 'N');
    renderColumn(2, "%c", packet.isRemote ? 'Y' : 'N');
    renderColumn(3, "%d", packet.dataLen);
    renderColumn(4, "%s", packet.isRemote ? &" "[0] : formatData(&packet.data[0], packet.dataLen).c_str());

    ImGui::PopID();
    ImGui::EndGroup();
}

}    // namespace Frasy
