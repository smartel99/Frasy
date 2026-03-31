/**
 * @file    usb_tree_viewer.h
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


#ifndef FRASY_SRC_LAYERS_USB_TREE_VIEWER_H
#define FRASY_SRC_LAYERS_USB_TREE_VIEWER_H
#include "Brigerad/Core/Layer.h"

#include "utils/usb_enumerator/external_hub_info.h"
#include "utils/usb_enumerator/host_controller_info.h"
#include "utils/usb_enumerator/root_hub_info.h"
#include "utils/usb_enumerator/usb_device_info.h"
#include "utils/usb_enumerator/usb_node.h"

#include <atomic>
#include <future>

namespace Frasy {
class UsbTreeViewer : public Brigerad::Layer {
public:
    UsbTreeViewer();
    ~UsbTreeViewer() override = default;

    void onImGuiRender() override;

    void SetVisibility(bool visibility) { m_isVisible = visibility; }

private:
    bool                   m_isVisible    = false;
    std::atomic<bool>      m_isRefreshing = false;
    std::future<void>      m_refreshFuture;
    std::vector<Usb::Node> m_nodes;
};
}    // namespace Frasy

#endif    // FRASY_SRC_LAYERS_USB_TREE_VIEWER_H
