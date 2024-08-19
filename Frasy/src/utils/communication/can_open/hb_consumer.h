/**
 * @file    hb_consumer.h
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


#ifndef FRASY_UTILS_COMMUNICATION_CAN_OPEN_HB_CONSUMER_H
#define FRASY_UTILS_COMMUNICATION_CAN_OPEN_HB_CONSUMER_H

#include <CO_HBconsumer.h>
#include <CO_NMT_Heartbeat.h>

#include <cstdint>

namespace Frasy::CanOpen {
class HbConsumer {
public:
             HbConsumer() = default;
    explicit HbConsumer(CO_HBconsumer_t* co, uint8_t nodeId) : m_co(co), m_index(getIdxByNodeId(nodeId)) {}

    /**
     * Get the current state of a heartbeat producer by the index in OD 0x1016.
     * @param idx Object sub index
     * @return The current heartbeat state for the requested index.
     */
    [[nodiscard]] CO_HBconsumer_state_t getState() const { return CO_HBconsumer_getState(m_co, m_index); }

    /**
     * Get the current NMT state oof a heartbeat producer by the index in OD 0x1016.
     *
     * NMT state is only available when heartbeat is enabled for tthis index!
     * @param idx Object sub index.
     * @return The current NMT state.
     */
    [[nodiscard]] CO_NMT_internalState_t getNmtState() const
    {
        CO_NMT_internalState_t state = {};
        if (CO_HBconsumer_getNmtState(m_co, m_index, &state) == 0) { return state; }
        return CO_NMT_UNKNOWN;
    }

    constexpr auto operator<=>(const HbConsumer&) const = default;

private:
    /**
     * Get the heartbeat producer object index by node ID.
     * @param nodeId Producer node ID
     * @return Index if found, -1 otherwise.
     */
    [[nodiscard]] int8_t getIdxByNodeId(uint8_t nodeId) const { return CO_HBconsumer_getIdxByNodeId(m_co, nodeId); }

    CO_HBconsumer_t* m_co    = nullptr;
    int8_t           m_index = -1;
};
}    // namespace Frasy::CanOpen

#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_HB_CONSUMER_H
