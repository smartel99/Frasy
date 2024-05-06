/**
 * @file    em.h
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


#ifndef FRASY_UTILS_COMMUNICATION_CAN_OPEN_EM_H
#define FRASY_UTILS_COMMUNICATION_CAN_OPEN_EM_H

#include "to_string.h"

#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdint>
#include <format>

namespace Frasy::CanOpen {
struct EmergencyMessage {
    using timestamp_t = std::chrono::time_point<std::chrono::system_clock>;

    uint8_t                 nodeId        = {};
    CO_EM_errorCode_t       errorCode     = {};
    CO_errorRegister_t      errorRegister = {};
    CO_EM_errorStatusBits_t errorStatus   = {};
    uint32_t                information   = {};
    timestamp_t             timestamp;
    bool                    isActive = true;
    timestamp_t             resolutionTime;    //! Time at which the error went from actitve to solved.

    constexpr auto operator<=>(const EmergencyMessage&) const = default;

    [[nodiscard]] bool isCritical() const
    {
        switch (errorStatus) {
            case CO_EM_NO_ERROR: return false;
            case CO_EM_CAN_BUS_WARNING: return false;
            case CO_EM_RXMSG_WRONG_LENGTH: return false;
            case CO_EM_RXMSG_OVERFLOW: return false;
            case CO_EM_RPDO_WRONG_LENGTH: return false;
            case CO_EM_RPDO_OVERFLOW: return false;
            case CO_EM_CAN_RX_BUS_PASSIVE: return false;
            case CO_EM_CAN_TX_BUS_PASSIVE: return false;
            case CO_EM_NMT_WRONG_COMMAND: return false;
            case CO_EM_TIME_TIMEOUT: return false;
            case CO_EM_0A_unused: return false;
            case CO_EM_0B_unused: return false;
            case CO_EM_0C_unused: return false;
            case CO_EM_0D_unused: return false;
            case CO_EM_0E_unused: return false;
            case CO_EM_0F_unused: return false;
            case CO_EM_10_unused: return true;
            case CO_EM_11_unused: return true;
            case CO_EM_CAN_TX_BUS_OFF: return true;
            case CO_EM_CAN_RXB_OVERFLOW: return true;
            case CO_EM_CAN_TX_OVERFLOW: return true;
            case CO_EM_TPDO_OUTSIDE_WINDOW: return true;
            case CO_EM_16_unused: return true;
            case CO_EM_RPDO_TIME_OUT: return true;
            case CO_EM_SYNC_TIME_OUT: return true;
            case CO_EM_SYNC_LENGTH: return true;
            case CO_EM_PDO_WRONG_MAPPING: return true;
            case CO_EM_HEARTBEAT_CONSUMER: return true;
            case CO_EM_HB_CONSUMER_REMOTE_RESET: return true;
            case CO_EM_1D_unused: return true;
            case CO_EM_1E_unused: return true;
            case CO_EM_1F_unused: return true;
            case CO_EM_EMERGENCY_BUFFER_FULL: return false;
            case CO_EM_21_unused: return false;
            case CO_EM_MICROCONTROLLER_RESET: return false;
            case CO_EM_23_unused: return false;
            case CO_EM_24_unused: return false;
            case CO_EM_25_unused: return false;
            case CO_EM_26_unused: return false;
            case CO_EM_NON_VOLATILE_AUTO_SAVE: return false;
            case CO_EM_WRONG_ERROR_REPORT: return true;
            case CO_EM_ISR_TIMER_OVERFLOW: return true;
            case CO_EM_MEMORY_ALLOCATION_ERROR: return true;
            case CO_EM_GENERIC_ERROR: return true;
            case CO_EM_GENERIC_SOFTWARE_ERROR: return true;
            case CO_EM_INCONSISTENT_OBJECT_DICT: return true;
            case CO_EM_CALCULATION_OF_PARAMETERS: return true;
            case CO_EM_NON_VOLATILE_MEMORY: return true;
            case CO_EM_MANUFACTURER_START: return false;
            case CO_EM_MANUFACTURER_END: return true;
            default:
                // Based on
                // https://canopennode.github.io/CANopenSocket/group__CO__Emergency.html#ga587034df9d350c8e121c253f1d4eeacc
                if (errorStatus >= 0x30 && errorStatus <= 0x3F) { return false; }
                if (errorStatus >= 0x40 && errorStatus <= 0x4F) { return true; }
                return true;
        }
    }
};
struct EmergencyMessageFormatter {
    template<typename ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx)
    {
        auto it = ctx.begin();
        if (it == ctx.end()) { return it; }
        if (it != ctx.end() && *it != '}') { throw std::format_error("Invalid format args for EmergencyMessage."); }

        return it;
    }

    template<typename FmtContext>
    FmtContext::iterator format(const EmergencyMessage& em, FmtContext& ctx) const
    {
        using CanOpen::toString;
        std::format_to(ctx.out(),
                       "Node ID: {}, {}, "
                       "\n\rCode: {},"
                       "\n\rRegister: {},"
                       "\n\rStatus: {}"
                       "\n\rInfo: 0x{:08x}"
                       "\n\rActive: {}, Resolution Time: {}",
                       em.nodeId,
                       em.timestamp,
                       toString(em.errorCode),
                       toString(em.errorRegister),
                       toString(em.errorStatus),
                       em.information,
                       em.isActive,
                       em.resolutionTime);

        return ctx.out();
    }
};
}    // namespace Frasy::CanOpen


template<>
struct std::formatter<Frasy::CanOpen::EmergencyMessage> : Frasy::CanOpen::EmergencyMessageFormatter {};

template<>
struct fmt::formatter<Frasy::CanOpen::EmergencyMessage> : Frasy::CanOpen::EmergencyMessageFormatter {};

#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_EM_H
