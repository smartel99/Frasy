/**
 * @file    device_viewer.h
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

#ifndef FRASY_LAYERS_DEVICE_VIEWER_H
#define FRASY_LAYERS_DEVICE_VIEWER_H

#include "utils/communication/can_open/can_open.h"
#include "utils/config.h"

#include <Brigerad.h>
#include <optional>
#include <unordered_map>

namespace Frasy {
class DeviceViewer : public Brigerad::Layer {
    struct DeviceViewerOptions {
        struct WhitelistItem {
            int                vid = 0;
            int                pid = 0;
            std::optional<int> rev;
            std::optional<int> mi;

            bool operator==(const WhitelistItem& rhs) const
            {
                return vid == rhs.vid && pid == rhs.pid && (!rev.has_value() || rev == rhs.rev) &&
                       (!mi.has_value() || mi == rhs.mi);
            }
            bool operator==(const std::string& rhs) const;

            static std::optional<WhitelistItem> parse(const std::string& name);
            static std::optional<WhitelistItem> parse(const std::wstring& name);
        };

        std::string                lastDevice;
        std::vector<WhitelistItem> usbWhitelist;
    };
    friend void to_json(nlohmann::json& j, const DeviceViewerOptions& options);
    friend void to_json(nlohmann::json& j, const DeviceViewerOptions::WhitelistItem& item);
    friend void from_json(const nlohmann::json& j, DeviceViewerOptions::WhitelistItem& item);
    friend void from_json(const nlohmann::json& j, DeviceViewerOptions& options);

public:
     DeviceViewer(CanOpen::CanOpen& canOpen) noexcept;
    ~DeviceViewer() override = default;

    void onAttach() override;
    void onDetach() override;
    void onEvent(Brigerad::Event& event) override;

    void onImGuiRender() override;

    void setVisibility(bool visibility);

private:
    void refreshPorts();

    void renderNetworkState();
    void renderNetworkPacket(const SlCan::CanPacket& packet);

    void renderMenuBar();

    void handleSerialConnection(const DeviceViewerOptions::WhitelistItem& info, const std::string& port);
    void handleSerialDisconnection(const std::string& port);

private:
    bool                m_isVisible = false;
    DeviceViewerOptions m_options;

    CanOpen::CanOpen&             m_canOpen;
    std::string                   m_selectedPort;
    std::vector<serial::PortInfo> m_ports;

    std::unordered_map<decltype(std::declval<SlCan::CanPacket>().id), SlCan::CanPacket> m_networkState;

    std::jthread m_resetter;
    size_t       m_pktRxCount               = 0;
    size_t       m_packetsRxInLastSecond    = 0;
    size_t       m_packetsRxInCurrentSecond = 0;

    float  m_kilobytesRxInLastSecond = 0;
    size_t m_bytesRxInCurrentSecond  = 0;

    size_t m_pktTxCount               = 0;
    size_t m_packetsTxInLastSecond    = 0;
    size_t m_packetsTxInCurrentSecond = 0;
    float  m_kilobytesTxInLastSecond  = 0;
    size_t m_bytesTxInCurrentSecond   = 0;

    static constexpr const char* s_windowName = "Devices";
};
}    // namespace Frasy
#endif    // FRASY_LAYERS_DEVICE_VIEWER_H
