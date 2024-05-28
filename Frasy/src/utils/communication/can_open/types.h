/**
 * @file    types.h
 * @author  Paul Thomas
 * @date    5/23/2024
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
#ifndef FRASY_SRC_UTILS_COMMUNICATION_CAN_OPEN_TYPES_H
#define FRASY_SRC_UTILS_COMMUNICATION_CAN_OPEN_TYPES_H

#include <cstdint>

namespace Frasy::CanOpen {

enum class DataType : uint16_t {
    boolean                   = 0x1,
    integer8                  = 0x2,
    integer16                 = 0x3,
    integer32                 = 0x4,
    unsigned8                 = 0x5,
    unsigned16                = 0x6,
    unsigned32                = 0x7,
    real32                    = 0x8,
    visibleString             = 0x9,
    octetString               = 0xa,
    unicodeString             = 0xb,
    timeOfDay                 = 0xc,
    timeDifference            = 0xd,
    domain                    = 0xf,
    integer24                 = 0x10,
    real64                    = 0x11,
    integer40                 = 0x12,
    integer48                 = 0x13,
    integer56                 = 0x14,
    integer64                 = 0x15,
    unsigned24                = 0x16,
    unsigned40                = 0x18,
    unsigned48                = 0x19,
    unsigned56                = 0x1A,
    unsigned64                = 0x1B,
    pdoCommunicationParameter = 0x20,
    pdoMapping                = 0x21,
    sdoParameter              = 0x22,
    identity                  = 0x23,
};

}

#endif    // FRASY_SRC_UTILS_COMMUNICATION_CAN_OPEN_TYPES_H
