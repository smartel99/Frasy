/**
 * @file    to_string.h
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


#ifndef FRASY_UTILS_COMMUNICATION_CAN_OPEN_TO_STRING_H
#define FRASY_UTILS_COMMUNICATION_CAN_OPEN_TO_STRING_H

#include <CO_HBconsumer.h>
#include <CO_NMT_Heartbeat.h>

#include <string_view>

namespace Frasy::CanOpen {
constexpr std::string_view toString(CO_HBconsumer_state_t state)
{
    switch (state) {
        case CO_HBconsumer_UNCONFIGURED: return "Unconfigured";
        case CO_HBconsumer_ACTIVE: return "Active";
        case CO_HBconsumer_TIMEOUT: return "Timeout";
        case CO_HBconsumer_UNKNOWN:
        default: return "Unknown";
    }
}

constexpr std::string_view toString(CO_NMT_internalState_t state)
{
    switch (state) {
        case CO_NMT_INITIALIZING: return "Initializing";
        case CO_NMT_PRE_OPERATIONAL: return "Pre-Operational";
        case CO_NMT_OPERATIONAL: return "Operational";
        case CO_NMT_STOPPED: return "Stopped";
        case CO_NMT_UNKNOWN:
        default: return "Unknown";
    }
}
}    // namespace Frasy::CanOpen

#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_TO_STRING_H
