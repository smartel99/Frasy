/**
 * @file    node.h
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


#ifndef FRASY_UTILS_COMMUNICATION_CAN_OPEN_NODE_H
#define FRASY_UTILS_COMMUNICATION_CAN_OPEN_NODE_H

#include "em.h"
#include "hb_consumer.h"
#include "services/sdo.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>

namespace Frasy::CanOpen {
class CanOpen;

class Node {
    friend CanOpen;

public:
    Node() = default;
    Node(CanOpen* canOpen, uint8_t nodeId, std::string_view name, std::string_view edsPath);

    [[nodiscard]] uint8_t          nodeId() const { return m_nodeId; }
    [[nodiscard]] std::string_view name() const { return m_name; }
    [[nodiscard]] CanOpen*         bus() const { return m_canOpen; }

    constexpr bool operator==(const Node& o) const
    {
        return m_nodeId == o.m_nodeId && m_name == o.m_name && m_edsPath == o.m_edsPath;
    }

    [[nodiscard]] CO_HBconsumer_state_t  getHbState() const { return m_hbConsumer.getState(); }
    [[nodiscard]] CO_NMT_internalState_t getNmtState() const { return m_hbConsumer.getNmtState(); }

    [[nodiscard]] SdoManager* sdoInterface() { return m_sdoManager.get(); }

    void                                               addEmergency(EmergencyMessage em);
    [[nodiscard]] const std::vector<EmergencyMessage>& getEmergencies() const { return m_emHistory; }

private:
    void setHbConsumer(CO_HBconsumer_t* hbConsumer);
    void setSdoClient(CO_SDOclient_t* client) { m_sdoManager->setSdoClient(client); }

    void removeHbConsumer() { m_hbConsumer = {}; }
    void removeSdoClient() { m_sdoManager->removeSdoClient(); }

private:
    uint8_t     m_nodeId = 0;
    std::string m_name;
    std::string m_edsPath;

    HbConsumer m_hbConsumer;
    std::unique_ptr<SdoManager> m_sdoManager;

    std::vector<EmergencyMessage> m_emHistory;

    CanOpen* m_canOpen = nullptr;
};
}    // namespace Frasy::CanOpen
#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_NODE_H
