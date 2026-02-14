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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "device_viewer.h"

#include "utils/communication/serial/device_map.h"
#include "utils/communication/serial/enumerator.h"

#include "Brigerad/Events/usb_event.h"
#include "frasy_interpreter.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <regex>
#include <string>
#include <utils/get_serial_port.h>
#include <vector>

namespace Frasy {


std::optional<DeviceViewer::DeviceViewerOptions::WhitelistItem> DeviceViewer::DeviceViewerOptions::WhitelistItem::parse(
  const std::string& name)
{
    WhitelistItem info {};
    std::regex    pVid("VID_([A-F0-9]+)");
    std::regex    pPid("PID_([A-F0-9]+)");
    std::regex    pMi("MI_([A-F0-9]+)");
    std::regex    pRev("REV_([A-F0-9]+)");
    std::smatch   match;
    if (!std::regex_search(name, match, pVid)) { return {}; }
    auto r = parseHexInteger<uint32_t>(match[1]);
    if (!r.has_value()) { return {}; }
    info.vid = r.value();
    if (!std::regex_search(name, match, pPid)) { return {}; }
    r = parseHexInteger<uint32_t>(match[1]);
    if (!r.has_value()) { return {}; }
    info.pid = r.value();
    if (std::regex_search(name, match, pMi)) {
        r = parseHexInteger<uint32_t>(match[1]);
        if (r.has_value()) { info.mi = r.value(); }
    }
    if (std::regex_search(name, match, pRev)) {
        r = parseHexInteger<uint32_t>(match[1]);
        if (r.has_value()) { info.rev = r.value(); }
    }
    return info;
}


std::optional<DeviceViewer::DeviceViewerOptions::WhitelistItem> DeviceViewer::DeviceViewerOptions::WhitelistItem::parse(
  const std::wstring& name)
{
    return parse(wstring_to_utf8(name));
}


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
    j = nlohmann::json {{"lastDevices", options.lastDevices}, {"usbWhitelist", options.usbWhitelist}};
}
void from_json(const nlohmann::json& j, DeviceViewer::DeviceViewerOptions& options)
{
    if (j.contains("lastDevices")) { j["lastDevices"].get_to(options.lastDevices); }
    if (j.contains("usbWhitelist")) { j["usbWhitelist"].get_to(options.usbWhitelist); }
}

DeviceViewer::DeviceViewer(CanOpen::CanOpen& canOpen) noexcept : m_canOpen(canOpen)
{
    for (auto& device : m_canOpen.m_devices | std::views::values) {
        device.m_rxMonitorFunc = [this](const SlCan::Packet& pkt) {
            FRASY_PROFILE_FUNCTION();
            m_pktRxCount++;

            m_packetsRxInCurrentSecond++;
            m_bytesRxInCurrentSecond += pkt.sizeOfSerialPacket();
            if (!m_isVisible) { return; }
            if (!commandIsTransmit(pkt.command)) { return; }

            m_networkState[pkt.data.packetData.id] = pkt.data.packetData;
        };

        device.m_txMonitorFunc = [this](const SlCan::Packet& pkt) {
            FRASY_PROFILE_FUNCTION();
            m_pktTxCount++;

            m_packetsTxInCurrentSecond++;
            m_bytesTxInCurrentSecond += pkt.sizeOfSerialPacket();
            if (!m_isVisible) { return; }
            if (!commandIsTransmit(pkt.command)) { return; }

            m_networkState[pkt.data.packetData.id] = pkt.data.packetData;
        };
    }

    m_resetter = std::jthread([this](std::stop_token stopToken) {
        using namespace std::chrono_literals;

        while (!stopToken.stop_requested()) {
            m_packetsRxInLastSecond    = (m_packetsRxInLastSecond + m_packetsRxInCurrentSecond) / 2;
            m_packetsRxInCurrentSecond = 0;

            m_kilobytesRxInLastSecond =
              (m_kilobytesRxInLastSecond + static_cast<float>(m_bytesRxInCurrentSecond) / 1024.0f) / 2.0f;
            m_bytesRxInCurrentSecond = 0;

            m_packetsTxInLastSecond    = (m_packetsTxInLastSecond + m_packetsTxInCurrentSecond) / 2;
            m_packetsTxInCurrentSecond = 0;

            m_kilobytesTxInLastSecond =
              (m_kilobytesTxInLastSecond + static_cast<float>(m_bytesTxInCurrentSecond) / 1024.0f) / 2.0f;
            m_bytesTxInCurrentSecond = 0;
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
    m_options = Interpreter::Get().getConfig().value("communication", DeviceViewerOptions {});
    m_ports   = serial::list_ports();

    auto isValidUsb = [&usbWhitelist = m_options.usbWhitelist](const serial::PortInfo& info) -> bool {
        return std::ranges::any_of(
          usbWhitelist, [&vidPid = info.hardware_id](const DeviceViewerOptions::WhitelistItem& item) -> bool {
              return item == vidPid;
          });
    };

    for (auto& port : m_ports | std::views::filter(isValidUsb)) {
        if (m_canOpen.addDevice(port.port)) {
            auto& device = m_canOpen.m_devices[port.port];

            device.m_rxMonitorFunc = [this](const SlCan::Packet& pkt) {
                FRASY_PROFILE_FUNCTION();
                m_pktRxCount++;

                m_packetsRxInCurrentSecond++;
                m_bytesRxInCurrentSecond += pkt.sizeOfSerialPacket();
                if (!m_isVisible) { return; }
                if (!commandIsTransmit(pkt.command)) { return; }

                m_networkState[pkt.data.packetData.id] = pkt.data.packetData;
            };

            device.m_txMonitorFunc = [this](const SlCan::Packet& pkt) {
                FRASY_PROFILE_FUNCTION();
                m_pktTxCount++;

                m_packetsTxInCurrentSecond++;
                m_bytesTxInCurrentSecond += pkt.sizeOfSerialPacket();
                if (!m_isVisible) { return; }
                if (!commandIsTransmit(pkt.command)) { return; }

                m_networkState[pkt.data.packetData.id] = pkt.data.packetData;
            };
        }
    }
}

void DeviceViewer::onDetach()
{
    // Save the currently opened device, if one exists.
    Interpreter::Get().getConfig()["communication"] = m_options;
}

void DeviceViewer::onEvent(Brigerad::Event& event)
{
    Brigerad::EventDispatcher dispatcher = Brigerad::EventDispatcher(event);
    dispatcher.Dispatch<Brigerad::UsbConnectedEvent>([&](Brigerad::Event& e) {
        const auto& usbEvent = reinterpret_cast<const Brigerad::UsbEvent&>(e);
        const auto  info     = DeviceViewerOptions::WhitelistItem::parse(usbEvent.name);
        if (!info.has_value()) {
            BR_APP_DEBUG("Could not parse device info, aborting");
            return false;
        }
        m_ports                = serial::list_ports();
        const std::string port = getSerialPort(usbEvent, m_ports);
        if (port.empty()) {
            BR_APP_DEBUG("Could not find port info, aborting");
            return false;
        }
        handleSerialConnection(info.value(), port);
        return false;
    });
    dispatcher.Dispatch<Brigerad::UsbDisconnectedEvent>([&](Brigerad::Event& e) {
        // We must used previously used ports because if our port was disconnected, we won't have it anymore
        const std::string port = getSerialPort(reinterpret_cast<const Brigerad::UsbEvent&>(e), m_ports);
        // We do not update m_ports in case of multiple callback in a row
        // m_ports                = serial::list_ports();
        if (port.empty()) {
            BR_APP_DEBUG("Could not find port info, aborting");
            return false;
        }
        handleSerialDisconnection(port);
        return false;
    });
}




void DeviceViewer::onImGuiRender()
{
    renderMenuBar();

    if (!m_isVisible) { return; }

    if (ImGui::Begin(s_windowName, &m_isVisible, ImGuiWindowFlags_NoDocking)) {
        if (m_canOpen.isOpen()) {
            ImGui::Text("Connected to: ");
            for (const auto& port : m_canOpen.m_devices | std::views::keys) {
                ImGui::SameLine();
                ImGui::Text("%s, ", port.c_str());
            }
            ImGui::SameLine();
            if (ImGui::Button("Close")) {
                for (auto& device : m_canOpen.m_devices | std::views::values) {
                    device.close();
                }
                m_selectedPort = "";
            }
        }
        else {
            if (ImGui::BeginCombo("Port", !m_selectedPort.empty() ? m_selectedPort.c_str() : "")) {
                for (auto&& port : m_ports) {
                    if (ImGui::Selectable(port.port.c_str(), port.port == m_selectedPort)) {
                        m_selectedPort = port.port;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (ImGui::Button("Refresh")) {
                std::string selectedPort = m_selectedPort.empty() ? "" : m_selectedPort;
                refreshPorts();
                if (!m_selectedPort.empty()) {
                    auto it = std::ranges::find_if(
                      m_ports, [&selectedPort](const auto& port) { return port.port == selectedPort; });
                    if (it != m_ports.end()) { m_selectedPort = it->port; }
                }
            }

            if (m_selectedPort.empty()) {
                const auto& disabled = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, disabled);
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, disabled);
                ImGui::PushStyleColor(ImGuiCol_FrameBgActive, disabled);
            }
            ImGui::SameLine();
            // if (ImGui::Button("Open") && !m_selectedPort.empty()) { m_canOpen.open(m_selectedPort); }
            if (m_selectedPort.empty()) { ImGui::PopStyleColor(3); }
        }
        ImGui::Separator();
        ImGui::Text(
          "Network State: %zu addresses, %zu pending packets (%zu total), %zu pkt/s, %0.3f kB/s",
          m_networkState.size(),
          [&devices = m_canOpen.m_devices]() -> size_t {
              size_t tot = 0;
              for (auto& device : devices | std::views::values) {
                  tot += device.m_queue.size();
              }
              return tot;
          }(),
          m_pktRxCount,
          m_packetsRxInLastSecond,
          m_kilobytesRxInLastSecond);
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            m_networkState.clear();
            m_pktRxCount = 0;
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
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        for (const auto& packet : m_networkState | std::views::values) {
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
        return std::format("{::#02x}", std::span {data, len});
    };

    ImGui::BeginGroup();
    // Use the pointer location as a unique ID.
    ImGui::PushID(static_cast<int>(packet.id));

    renderColumn(0, "0x%08x", packet.id);
    renderColumn(1, "%c", packet.isExtended ? 'Y' : 'N');
    renderColumn(2, "%c", packet.isRemote ? 'Y' : 'N');
    renderColumn(3, "%d", packet.dataLen);
    renderColumn(4, "%s", packet.isRemote ? &" "[0] : formatData(&packet.data[0], packet.dataLen).c_str());

    ImGui::PopID();
    ImGui::EndGroup();
}

void DeviceViewer::renderMenuBar()
{
    if (ImGui::BeginMainMenuBar()) {
        ImGui::BeginHorizontal("deviceViewerMenuBarSpan", ImVec2 {ImGui::GetContentRegionAvail().x, 0.0f});
        ImGui::Spring(0.1f);
        ImGui::Separator();

        static constexpr ImVec4 s_red   = {1, 0, 0, 1};
        static constexpr ImVec4 s_green = {0, 1, 0, 1};

        if (m_canOpen.m_redLed) { ImGui::PushStyleColor(ImGuiCol_Text, s_red); }
        else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        }
        ImGui::Bullet();
        ImGui::PopStyleColor();

        ImGui::Spring(0);

        if (m_canOpen.m_greenLed) { ImGui::PushStyleColor(ImGuiCol_Text, s_green); }
        else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        }
        ImGui::Bullet();
        ImGui::PopStyleColor();

        ImGui::Spring(0);

        static constexpr ImVec4 s_connectedColor    = {40, 255, 0, 255};
        static constexpr ImVec4 s_disconnectedColor = {255, 0, 0, 255};
        ImVec4                  color               = m_canOpen.isOpen() ? s_connectedColor : s_disconnectedColor;
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        if (m_canOpen.isOpen()) { ImGui::Text("Connected (%d)  ", static_cast<int>(m_canOpen.m_devices.size())); }
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

            // Make at least a tiny bit of color show up if there's some activity.
            if (usage > 0.0001f) { usage = std::max(usage, 0.02f); }
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, progressColor);
            ImGui::ProgressBar(usage, ImVec2 {50.0f, 0.0f}, "");
            ImGui::PopStyleColor();
        };

        // Combine both Rx and Tx into one bar, then expand in the pop up.
        renderNetworkUsage((m_kilobytesRxInLastSecond + m_kilobytesTxInLastSecond) / 300.0f);

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Addresses: %zu", m_networkState.size());
            ImGui::Text("Packets Pending: %zu", [&devices = m_canOpen.m_devices]() -> size_t {
                size_t tot = 0;
                for (auto&& [port, device] : devices) {
                    tot += device.m_queue.size();
                }
                return tot;
            }());
            ImGui::Text("Packets Received: %zu", m_pktRxCount);
            ImGui::Text("Packets Sent: %zu", m_pktTxCount);
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
}

void DeviceViewer::handleSerialConnection(const DeviceViewerOptions::WhitelistItem& info, const std::string& port)
{
    BR_APP_DEBUG("Handling connection of port {}", port);
    if (port.empty()) {
        BR_APP_DEBUG("No port provided, aborting");
        return;
    }
    if (!std::ranges::any_of(m_options.usbWhitelist, [info](const auto& device) { return device == info; })) {
        BR_APP_DEBUG("Not in whitelist, aborting");
        return;
    }
    BR_APP_INFO("Connecting to {}", port);
    if (m_canOpen.addDevice(port)) {
        BR_APP_INFO("Connected");
        // add monitoring hooks.
        auto& device           = m_canOpen.m_devices[port];
        device.m_rxMonitorFunc = [this](const SlCan::Packet& pkt) {
            FRASY_PROFILE_FUNCTION();
            m_pktRxCount++;

            m_packetsRxInCurrentSecond++;
            m_bytesRxInCurrentSecond += pkt.sizeOfSerialPacket();
            if (!m_isVisible) { return; }
            if (!commandIsTransmit(pkt.command)) { return; }

            m_networkState[pkt.data.packetData.id] = pkt.data.packetData;
        };

        device.m_txMonitorFunc = [this](const SlCan::Packet& pkt) {
            FRASY_PROFILE_FUNCTION();
            m_pktTxCount++;

            m_packetsTxInCurrentSecond++;
            m_bytesTxInCurrentSecond += pkt.sizeOfSerialPacket();
            if (!m_isVisible) { return; }
            if (!commandIsTransmit(pkt.command)) { return; }

            m_networkState[pkt.data.packetData.id] = pkt.data.packetData;
        };
    }
}

void DeviceViewer::handleSerialDisconnection(const std::string& port)
{
    BR_APP_DEBUG("Handling disconnection of port {}", port);
    if (port.empty()) {
        BR_APP_DEBUG("No port provided, aborting");
        return;
    }
    BR_APP_DEBUG("Removing port {} from the list...", port);
    m_canOpen.removeDevice(port);
    m_selectedPort = "";
}



}    // namespace Frasy
