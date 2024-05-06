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

#include <spdlog/spdlog.h>

#include <format>
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

constexpr std::string_view toString(CO_EM_errorCode_t code)
{
    switch (code) {
        case CO_EMC_NO_ERROR: return "No Error";
        case CO_EMC_GENERIC: return "Generic";
        case CO_EMC_CURRENT: return "Current";
        case CO_EMC_CURRENT_INPUT: return "Current - Input Side";
        case CO_EMC_CURRENT_INSIDE: return "Current - On Board";
        case CO_EMC_CURRENT_OUTPUT: return "Current - Output Side";
        case CO_EMC_VOLTAGE: return "Voltage";
        case CO_EMC_VOLTAGE_MAINS: return "Voltage - Input Side";
        case CO_EMC_VOLTAGE_INSIDE: return "Voltage - On Board";
        case CO_EMC_VOLTAGE_OUTPUT: return "Voltage - Output Side";
        case CO_EMC_TEMPERATURE: return "Temperature";
        case CO_EMC_TEMP_AMBIENT: return "Ambiant Temperature";
        case CO_EMC_TEMP_DEVICE: return "Device Temperature";
        case CO_EMC_HARDWARE: return "Hardware";
        case CO_EMC_SOFTWARE_DEVICE: return "Software - Device";
        case CO_EMC_SOFTWARE_INTERNAL: return "Software - Internal";
        case CO_EMC_SOFTWARE_USER: return "Software - User";
        case CO_EMC_DATA_SET: return "Software - Data Set";
        case CO_EMC_ADDITIONAL_MODUL: return "Additional Modules";
        case CO_EMC_MONITORING: return "Monitoring";
        case CO_EMC_COMMUNICATION: return "Monitoring - Communication";
        case CO_EMC_CAN_OVERRUN: return "Monitoring - CAN Overrun";
        case CO_EMC_CAN_PASSIVE: return "Monitoring - CAN Passive";
        case CO_EMC_HEARTBEAT: return "Monitoring - Heartbeat";
        case CO_EMC_BUS_OFF_RECOVERED: return "Monitoring - Bus Off Recovered";
        case CO_EMC_CAN_ID_COLLISION: return "Monitoring - CAN ID Collision";
        case CO_EMC_PROTOCOL_ERROR: return "Monitoring - Protocol Error";
        case CO_EMC_PDO_LENGTH: return "Monitoring - PDO not processed due to length error";
        case CO_EMC_PDO_LENGTH_EXC: return "Monitoring - PDO length exceeded";
        case CO_EMC_DAM_MPDO: return "Monitoring - DAM MPDO not processed, destination object not available";
        case CO_EMC_SYNC_DATA_LENGTH: return "Monitoring - Unexpected SYNC data length";
        case CO_EMC_RPDO_TIMEOUT: return "Monitoring - RPDO timeout";
        case CO_EMC_EXTERNAL_ERROR: return "Extetrnal error";
        case CO_EMC_ADDITIONAL_FUNC: return "Additional functions";
        case CO_EMC_DEVICE_SPECIFIC: return "Device Specific";
        case CO_EMC401_OUT_CUR_HI: return "DS401 - Current at outputs ttoo high (overload)";
        case CO_EMC401_OUT_SHORTED: return "DS401 - Short circuit at outputs";
        case CO_EMC401_OUT_LOAD_DUMP: return "DS401 - Load dump at outputs";
        case CO_EMC401_IN_VOLT_HI: return "DS401 - Input voltage too high";
        case CO_EMC401_IN_VOLT_LOW: return "DS401 - Input voltage too low";
        case CO_EMC401_INTERN_VOLT_HI: return "DS401 - Internal voltage too high";
        case CO_EMC401_INTERN_VOLT_LO: return "DS401 - Internal voltage too low";
        case CO_EMC401_OUT_VOLT_HIGH: return "DS401 - Output voltage too high";
        case CO_EMC401_OUT_VOLT_LOW: return "DS401 - Output voltage too low";
        default: return "Unknown";
    }
}

constexpr std::string_view toString(CO_errorRegister_t reg)
{
    switch (reg) {

        case CO_ERR_REG_GENERIC_ERR: return "Generic Error";
        case CO_ERR_REG_CURRENT: return "Current";
        case CO_ERR_REG_VOLTAGE: return "Voltage";
        case CO_ERR_REG_TEMPERATURE: return "Temperature";
        case CO_ERR_REG_COMMUNICATION: return "Communication";
        case CO_ERR_REG_DEV_PROFILE: return "Device Profile";
        case CO_ERR_REG_RESERVED: return "Reserved";
        case CO_ERR_REG_MANUFACTURER: return "Manufacturer";
        default: return "Unknown";
    }
}

constexpr std::string_view toString(CO_EM_errorStatusBits_t status)
{
    switch (status) {

        case CO_EM_NO_ERROR: return "No Error";
        case CO_EM_CAN_BUS_WARNING: return "CAN bus warning limit reached";
        case CO_EM_RXMSG_WRONG_LENGTH: return "Wrong data length of the received CAN message";
        case CO_EM_RXMSG_OVERFLOW: return "Previous received CAN message wasn't processed yet";
        case CO_EM_RPDO_WRONG_LENGTH: return "Wrong data length of received PDO";
        case CO_EM_RPDO_OVERFLOW: return "Previous received PDO wasn't processed yet";
        case CO_EM_CAN_RX_BUS_PASSIVE: return "CAN receive bus is passive";
        case CO_EM_CAN_TX_BUS_PASSIVE: return "CAN transmit bus is passive";
        case CO_EM_NMT_WRONG_COMMAND: return "Wrong NMT command received";
        case CO_EM_TIME_TIMEOUT: return "TIME message timeout";
        case CO_EM_0A_unused:
            return "Unexpected TIME data length";    // It's actually used according to
                                                     // https://canopennode.github.io/CANopenSocket/group__CO__Emergency.html#ga587034df9d350c8e121c253f1d4eeacc
        case CO_EM_0B_unused: return "Unused - 0B";
        case CO_EM_0C_unused: return "Unused - 0C";
        case CO_EM_0D_unused: return "Unused - 0D";
        case CO_EM_0E_unused: return "Unused - 0E";
        case CO_EM_0F_unused: return "Unused - 0F";
        case CO_EM_10_unused: return "Unused - 10";
        case CO_EM_11_unused: return "Unused - 11";
        case CO_EM_CAN_TX_BUS_OFF: return "CAN transmit bus is off";
        case CO_EM_CAN_RXB_OVERFLOW: return "CAN module receive buffer has overflowed";
        case CO_EM_CAN_TX_OVERFLOW: return "CAN transmitt buffer has overflowed";
        case CO_EM_TPDO_OUTSIDE_WINDOW: return "TPDO is outside SYNC window";
        case CO_EM_16_unused: return "Unused - 16";
        case CO_EM_RPDO_TIME_OUT: return "RPDO timed out";
        case CO_EM_SYNC_TIME_OUT: return "SYNC message timeout";
        case CO_EM_SYNC_LENGTH: return "Unexpected SYNC data length";
        case CO_EM_PDO_WRONG_MAPPING: return "Error with PDO mapping";
        case CO_EM_HEARTBEAT_CONSUMER: return "Heartbeat consumer timeout";
        case CO_EM_HB_CONSUMER_REMOTE_RESET: return "Heartbeat consumer detected remote node reset";
        case CO_EM_1D_unused: return "Unused - 1D";
        case CO_EM_1E_unused: return "Unused - 1E";
        case CO_EM_1F_unused: return "Unused - 1F";
        case CO_EM_EMERGENCY_BUFFER_FULL: return "Emergency buffer is full, Emergency message wasn't sent";
        case CO_EM_21_unused: return "Unused - 21";
        case CO_EM_MICROCONTROLLER_RESET: return "Microcontroller has just started";
        case CO_EM_23_unused: return "Unused - 23";
        case CO_EM_24_unused: return "Unused - 24";
        case CO_EM_25_unused: return "Unused - 25";
        case CO_EM_26_unused: return "Unused - 26";
        case CO_EM_NON_VOLATILE_AUTO_SAVE: return "Non-Volatile auto save error";
        case CO_EM_WRONG_ERROR_REPORT: return "Wrong parameters to CO_errorReport() function";
        case CO_EM_ISR_TIMER_OVERFLOW: return "Timer task has overflowed";
        case CO_EM_MEMORY_ALLOCATION_ERROR: return "Unable to allocate memory for objects";
        case CO_EM_GENERIC_ERROR: return "Generic error, test usage";
        case CO_EM_GENERIC_SOFTWARE_ERROR: return "Generic software error";
        case CO_EM_INCONSISTENT_OBJECT_DICT: return "Object dictionary does not match the software";
        case CO_EM_CALCULATION_OF_PARAMETERS: return "Error in the calculation of the device's parameters";
        case CO_EM_NON_VOLATILE_MEMORY: return "Error with access to non-volatile device memory";
        case CO_EM_MANUFACTURER_START:
        case CO_EM_MANUFACTURER_END: return "Manufacturer";
        default:
            if (status > CO_EM_MANUFACTURER_START && status < CO_EM_MANUFACTURER_END) { return "Manufacturer"; }
            return "Unknwown";
    }
}

template<typename T>
struct Formatter {
    template<typename ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx)
    {
        auto it = ctx.begin();
        if (it == ctx.end()) { return it; }
        if (it != ctx.end() && *it != '}') { throw std::format_error("Invalid format args."); }

        return it;
    }

    template<typename FmtContext>
    FmtContext::iterator format(const T& t, FmtContext& ctx) const
    {
        using CanOpen::toString;
        std::format_to(ctx.out(), "{}", toString(t));

        return ctx.out();
    }
};

template<typename T>
concept Formattable = requires(T t)
{
    {
        CanOpen::toString(t)
    } -> std::same_as<std::string_view>;
};
}    // namespace Frasy::CanOpen

template<Frasy::CanOpen::Formattable T>
struct std::formatter<T> : Frasy::CanOpen::Formatter<T> {};

template<Frasy::CanOpen::Formattable T>
struct fmt::formatter<T> : Frasy::CanOpen::Formatter<T> {};

#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_TO_STRING_H
