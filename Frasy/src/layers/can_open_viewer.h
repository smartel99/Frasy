/**
 * @file    can_open_viewer.h
 * @author  Samuel Martel
 * @date    2024-04-29
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


#ifndef FRASY_SRC_LAYERS_CAN_OPEN_VIEWER_H
#define FRASY_SRC_LAYERS_CAN_OPEN_VIEWER_H

#include "utils/communication/can_open/can_open.h"
#include "utils/config.h"

#include <Brigerad.h>
#include <cstdint>

namespace Frasy {
class CanOpenViewer : public Brigerad::Layer {
    struct OpenNode {
        bool        open = false;
        std::string tabBarName;
        uint8_t     nodeId = 0;
    };

public:
     CanOpenViewer(CanOpen::CanOpen& canOpen) noexcept;
    ~CanOpenViewer() override = default;

    void onAttach() override;
    void onDetach() override;

    void onUpdate(Brigerad::Timestep timestep) override;
    void onImGuiRender() override;

    void setVisibility(bool visibility);

private:
    void renderNodes();
    bool renderOpenNodeWindow(OpenNode& node);
    void renderActiveErrors(const CanOpen::Node& node);
    void renderErrorHistory(const CanOpen::Node& node);

private:
    bool m_isVisible = false;

    CanOpen::CanOpen& m_canOpen;

    std::vector<OpenNode> m_openNodes;

    static constexpr const char* s_windowName             = "CANopen Viewer";
    static constexpr uint32_t    s_criticalEmergencyColor = static_cast<uint32_t>(0x203030DC);
};
}    // namespace Frasy
#endif    // FRASY_SRC_LAYERS_CAN_OPEN_VIEWER_H
