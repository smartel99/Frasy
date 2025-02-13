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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "node.h"

#include "can_open.h"

#include <cstdint>
#include <string_view>

namespace Frasy::CanOpen {

Node::Node(CanOpen* canOpen, uint8_t nodeId, std::string_view name, std::string_view edsPath)
: m_nodeId(nodeId),
  m_name(name),
  m_edsPath(edsPath),
  m_sdoManager(std::make_unique<SdoManager>(nodeId)),
  m_canOpen(canOpen)
{
}

void Node::reset() const
{
    m_canOpen->resetNode(m_nodeId);
}


void Node::addEmergency(EmergencyMessage em)
{
    if (em.errorCode == CO_EMC_NO_ERROR) {
        // NO_ERROR messages indicates that the error condition is now solved.
        // Find the first error in our history that has the same status, mark it as solved and save the time of
        // resolution.
        auto it = std::ranges::find_if(m_emHistory, [&em](const auto& message) {
            return message.errorStatus == em.errorStatus && message.isActive;
        });
        if (it == m_emHistory.end()) {
            // Not a message intended to clear an active error, but rather an active error itself. Might not actually be
            m_emHistory.push_back(em);
            return;
        }

        it->isActive       = false;
        it->resolutionTime = EmergencyMessage::timestamp_t::clock::now();
    }
    else {
        m_emHistory.push_back(em);
    }
}

void Node::setHbConsumer(CO_HBconsumer_t* hbConsumer)
{
    m_hbConsumer = HbConsumer {hbConsumer, m_nodeId};
}
}    // namespace Frasy::CanOpen
