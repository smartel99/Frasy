/**
 * @file    usb_node.h
 * @author  Sam Martel
 * @date    2026-02-26
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_USB_NODE_H
#define FRASY_UTILS_USB_ENUMERATOR_USB_NODE_H

#include <variant>

namespace Frasy::Usb {
struct HostControllerInfo;
struct RootHubInfo;
struct ExternalHubInfo;
struct DeviceInfo;

using Node = std::variant<HostControllerInfo, RootHubInfo, ExternalHubInfo, DeviceInfo>;
}

#endif //FRASY_UTILS_USB_ENUMERATOR_USB_NODE_H
