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

#include "can_open_viewer/open_node.h"
#include "utils/communication/can_open/can_open.h"

#include <Brigerad.h>
#include <cstdint>
#include <vector>

namespace Frasy::CanOpenViewer {
class Layer : public Brigerad::Layer {
public:
    explicit Layer(CanOpen::CanOpen& canOpen) noexcept;
    ~        Layer() override = default;

    void onAttach() override;
    void onDetach() override;

    void onUpdate(Brigerad::Timestep timestep) override;
    void onImGuiRender() override;

    void setVisibility(bool visibility);

private:
    void renderNodes();
    void renderErrorGenerator();

    bool m_isVisible = false;

    bool                    m_shouldRenderErrorGenerator = false;
    CO_EM_errorStatusBits_t m_selectedErrorKind          = CO_EM_NO_ERROR;
    CO_EM_errorCode_t       m_selectedErrorCode          = CO_EMC_NO_ERROR;
    std::array<char, 9>     m_selectedErrorInfo          = {'0', '0', '0', '0', '0', '0', '0', '0', 0};
    bool                    m_selectedErrorIsActive      = true;

    CanOpen::CanOpen& m_canOpen;

    std::vector<OpenNode> m_openNodes;

    static constexpr const char* s_windowName = "CANopen Viewer";
};
}    // namespace Frasy::CanOpenViewer
#endif    // FRASY_SRC_LAYERS_CAN_OPEN_VIEWER_H
