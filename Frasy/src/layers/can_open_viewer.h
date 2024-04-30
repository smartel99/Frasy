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

namespace Frasy {
class CanOpenViewer : public Brigerad::Layer {
public:
    CanOpenViewer(CanOpen::CanOpen& canOpen) noexcept;
    ~CanOpenViewer() override = default;

    void onAttach() override;
    void onDetach() override;

    void onImGuiRender() override;

    void setVisibility(bool visibility);

private:
    bool m_isVisible = false;

    CanOpen::CanOpen& m_canOpen;

    static constexpr const char* s_windowName = "CANopen Viewer";
};
}    // namespace Frasy
#endif    // FRASY_SRC_LAYERS_CAN_OPEN_VIEWER_H
