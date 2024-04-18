/**
 * @file    bitrates.h
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


#ifndef CEP_SLCAN_ENUMS_BITRATES_H
#define CEP_SLCAN_ENUMS_BITRATES_H

#include <cstdint>

namespace Frasy::SlCan {
enum class BitRates : uint8_t { b10k = 0, b20k, b50k, b100k, b125k, b250k, b500k, b750k, b1000k, bInvalid };

constexpr const char* bitRateToStr(BitRates bitRate)
{
    switch (bitRate) {
        case BitRates::b10k: return "10k";
        case BitRates::b20k: return "20k";
        case BitRates::b50k: return "50k";
        case BitRates::b100k: return "100k";
        case BitRates::b125k: return "125k";
        case BitRates::b250k: return "250k";
        case BitRates::b500k: return "500k";
        case BitRates::b750k: return "750k";
        case BitRates::b1000k: return "1000k";
        case BitRates::bInvalid:
        default: return "Invalid";
    }
}

constexpr BitRates bitRateFromChar(uint8_t val)
{
    BitRates rate = static_cast<BitRates>(val);
    switch (rate) {
        case BitRates::b10k:
        case BitRates::b20k:
        case BitRates::b50k:
        case BitRates::b100k:
        case BitRates::b125k:
        case BitRates::b250k:
        case BitRates::b500k:
        case BitRates::b750k:
        case BitRates::b1000k: return rate;
        case BitRates::bInvalid:
        default: return BitRates::bInvalid;
    }
}
}    // namespace SlCan

#endif    // CEP_SLCAN_ENUMS_BITRATES_H
