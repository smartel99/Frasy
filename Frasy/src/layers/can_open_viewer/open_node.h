/**
 * @file    open_node.h
 * @author  Samuel Martel
 * @date    2024-05-08
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


#ifndef FRASY_SRC_LAYERS_CAN_OPEN_VIEWER_OPEN_NODE_H
#define FRASY_SRC_LAYERS_CAN_OPEN_VIEWER_OPEN_NODE_H

#include "utils/communication/can_open/can_open.h"
#include "utils/communication/can_open/services/sdo_uploader.h"

#include "sdo.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace Frasy::CanOpenViewer {



class OpenNode {
public:
              OpenNode(uint8_t nodeId, CanOpen::CanOpen* canOpen);

    void onImGuiRender();

    [[nodiscard]] bool    open() const { return m_open; }
    [[nodiscard]] uint8_t nodeId() const { return m_nodeId; }

private:
    void renderActiveErrors(const CanOpen::Node& node);
    void renderErrorHistory(const CanOpen::Node& node);

private:
    uint8_t           m_nodeId = 0;
    CanOpen::CanOpen* m_canOpen;
    std::string       m_tabBarName;
    bool              m_open = true;

    std::unique_ptr<Sdo> m_sdo;

    static constexpr uint32_t s_criticalEmergencyColor = 0x203030DC;    // Pretty red uwu.
};
}    // namespace Frasy::CanOpenViewer

#endif    // FRASY_SRC_LAYERS_CAN_OPEN_VIEWER_OPEN_NODE_H
