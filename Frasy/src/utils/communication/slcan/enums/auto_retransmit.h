/**
 * @file    auto_retransmit.h
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


#ifndef CEP_SLCAN_ENUMS_AUTO_RETRANSMIT_H
#define CEP_SLCAN_ENUMS_AUTO_RETRANSMIT_H

#include <cstdint>

namespace Frasy::SlCan {
enum class AutoRetransmit : uint8_t { Disabled = 0, Enabled, Invalid };

constexpr const char* autoRetransmitToStr(AutoRetransmit autoRetransmit)
{
    switch (autoRetransmit) {
        case AutoRetransmit::Disabled: return "Disabled";
        case AutoRetransmit::Enabled: return "Enabled";
        case AutoRetransmit::Invalid:
        default: return "Invalid";
    }
}

constexpr AutoRetransmit autoRetransmitFromStr(uint8_t val)
{
    AutoRetransmit retransmit = static_cast<AutoRetransmit>(val);
    switch (retransmit) {
        case AutoRetransmit::Disabled:
        case AutoRetransmit::Enabled: return retransmit;
        case AutoRetransmit::Invalid:
        default: return AutoRetransmit::Invalid;
    }
}
}    // namespace SlCan
#endif    // CEP_SLCAN_ENUMS_AUTO_RETRANSMIT_H
