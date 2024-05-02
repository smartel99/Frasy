/**
 * @file    node.cpp
 * @author  Samuel Martel
 * @date    2024-05-02
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
#include "node.h"

#include "can_open.h"

#include <cstdint>
#include <string_view>

namespace Frasy::CanOpen {

Node::Node(CanOpen* canOpen, uint8_t nodeId, std::string_view name, std::string_view edsPath)
: m_nodeId(nodeId), m_name(name), m_edsPath(edsPath), m_hbConsumer(canOpen->getHbConsumer(nodeId)), m_canOpen(canOpen)
{
}
}    // namespace Frasy::CanOpen
