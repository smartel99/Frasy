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

#include "utils/communication/serial/device_map.h"
#include "utils/communication/serial/enumerator.h"

#include <atomic>
#include <Brigerad.h>

namespace Frasy
{
class DeviceViewer : public Brigerad::Layer
{
public:
    DeviceViewer() noexcept;
    ~DeviceViewer() override = default;

    void OnImGuiRender() override;

    void SetVisibility(bool visibility);

private:
    void        RenderDeviceList();
    static void RenderDeviceCommands(const std::unordered_map<Actions::cmd_id_t, Actions::CommandInfo::Reply>& commands,
                                     const Type::Manager&                                                      manager);
    static void RenderDeviceEnums(const std::unordered_map<Actions::cmd_id_t, Type::Enum>& enums);
    static void RenderDeviceStructs(const std::unordered_map<Actions::cmd_id_t, Type::Struct>& structs,
                                    const Type::Manager&                                       manager);
    static void RenderCommandValues(std::string_view                   name,
                                    const std::vector<Actions::Value>& values,
                                    const Type::Manager&               manager);

private:
    bool m_isVisible = false;

    Communication::DeviceMap& m_deviceMap;
    std::future<size_t>       m_scanResult;

    static constexpr const char* s_windowName = "Devices";
};
}    // namespace Frasy
#endif    // FRASY_LAYERS_DEVICE_VIEWER_H
