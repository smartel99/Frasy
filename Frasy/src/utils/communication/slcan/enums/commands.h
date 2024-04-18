/**
 * @file    commands.h
 * @author  Samuel Martel
 * @date    2024-04-09
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


#ifndef CEP_SLCAN_ENUMS_COMMANDS_H
#define CEP_SLCAN_ENUMS_COMMANDS_H

#include <cstdint>

namespace Frasy::SlCan {
enum class Command : uint8_t {
    OpenChannel            = 'O',
    CloseChannel           = 'C',
    SetBitRate             = 'S',
    SetMode                = 'M',
    SetAutoRetry           = 'A',
    GetVersion             = 'V',
    ReportError            = 'E',
    TransmitDataFrame      = 't',
    TransmitExtDataFrame   = 'T',
    TransmitRemoteFrame    = 'r',
    TransmitExtRemoteFrame = 'R',
    Invalid                = '\0'
};

constexpr const char* commandToStr(Command cmd)
{
    switch (cmd) {
        case Command::OpenChannel: return "Open Channel";
        case Command::CloseChannel: return "Close Channel";
        case Command::SetBitRate: return "Set Bit Rate";
        case Command::SetMode: return "Set Mode";
        case Command::SetAutoRetry: return "Set Auto Retry";
        case Command::GetVersion: return "Get Version";
        case Command::ReportError: return "Report Error";
        case Command::TransmitDataFrame: return "Transmit Data Frame";
        case Command::TransmitExtDataFrame: return "Transmit Extended Data Frame";
        case Command::TransmitRemoteFrame: return "Transmit Remote Frame";
        case Command::TransmitExtRemoteFrame: return "Transmit Extended Remote Frame";
        case Command::Invalid:
        default: return "Invalid";
    }
}

constexpr Command commandFromChar(uint8_t val)
{
    Command cmd = static_cast<Command>(val);
    switch (cmd) {
        case Command::OpenChannel:
        case Command::CloseChannel:
        case Command::SetBitRate:
        case Command::SetMode:
        case Command::SetAutoRetry:
        case Command::GetVersion:
        case Command::ReportError:
        case Command::TransmitDataFrame:
        case Command::TransmitExtDataFrame:
        case Command::TransmitRemoteFrame:
        case Command::TransmitExtRemoteFrame: return cmd;
        case Command::Invalid:
        default: return Command::Invalid;
    }
}

constexpr bool commandIsTransmit(Command cmd)
{
    switch (cmd) {
        case Command::TransmitDataFrame:
        case Command::TransmitExtDataFrame:
        case Command::TransmitRemoteFrame:
        case Command::TransmitExtRemoteFrame: return true;
        case Command::OpenChannel:
        case Command::CloseChannel:
        case Command::SetBitRate:
        case Command::SetMode:
        case Command::SetAutoRetry:
        case Command::GetVersion:
        case Command::ReportError:
        case Command::Invalid:
        default: return false;
    }
}
}    // namespace SlCan
#endif    // CEP_SLCAN_ENUMS_COMMANDS_H
