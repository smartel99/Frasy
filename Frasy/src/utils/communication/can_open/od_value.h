/**
 * @file    od_value.h
 * @author  Paul Thomas
 * @date    2026-11-19
 * @brief   Conversion between the raw bytes of an SDO transfer and their textual representation.
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this
 * program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef FRASY_UTILS_COMMUNICATION_CAN_OPEN_OD_VALUE_H
#define FRASY_UTILS_COMMUNICATION_CAN_OPEN_OD_VALUE_H

#include "utils/communication/can_open/services/var_type.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Frasy::CanOpen {
struct OdEntry;
}

namespace Frasy::CanOpen::OdValue {

/**
 * @brief Size, in bytes, of a fixed-size type.
 * @return The size, or 0 for the types that have no fixed size (strings and raw data).
 */
[[nodiscard]] constexpr std::size_t sizeOf(VarType type) noexcept
{
    switch (type) {
        case VarType::Boolean:
        case VarType::Signed8:
        case VarType::Unsigned8: return 1;
        case VarType::Signed16:
        case VarType::Unsigned16: return 2;
        case VarType::Signed32:
        case VarType::Unsigned32:
        case VarType::Real32: return 4;
        case VarType::Signed64:
        case VarType::Unsigned64:
        case VarType::Real64: return 8;
        case VarType::Undefined:
        case VarType::String:
        case VarType::Max:
        default: return 0;
    }
}

/**
 * @brief Name of a type, as it should be shown to the user.
 */
[[nodiscard]] std::string_view toString(VarType type) noexcept;

/**
 * @brief Converts the bytes received from a SDO upload to a human-readable string.
 *
 * @param type How the bytes should be interpreted. Types without a fixed size, as well as bytes whose size does not
 * match the type, are shown as hexadecimal bytes.
 * @param bytes Data received from the device.
 * @param withHex When true, unsigned integers are followed by their hexadecimal representation.
 */
[[nodiscard]] std::string format(VarType type, std::span<const uint8_t> bytes, bool withHex = true);

/**
 * @brief Converts text typed by the user into the bytes to send in a SDO download.
 *
 * Integers accept decimal and `0x` prefixed hexadecimal notations. Raw data (VarType::Undefined) is a sequence of
 * hexadecimal bytes, optionally separated by spaces or commas. Strings are sent with a terminating null character.
 *
 * @return The bytes, or the reason why the text is not acceptable for this type.
 */
[[nodiscard]] std::expected<std::vector<uint8_t>, std::string> parse(VarType type, std::string_view text);

/**
 * @brief Converts text typed by the user into the bytes to send to an entry of the object dictionary.
 *
 * Same as parse(VarType, std::string_view), except for the strings, whose size is known from the entry:
 * - VISIBLE_STRING: the characters of the text.
 * - OCTET_STRING: the bytes listed by the text, see parseOctetString().
 *
 * Both are padded with null bytes up to the `StringLengthMin` of the entry, and are rejected when longer. An entry
 * without `StringLengthMin` only accepts an empty value.
 */
[[nodiscard]] std::expected<std::vector<uint8_t>, std::string> parse(const OdEntry& entry, std::string_view text);

/**
 * @brief Converts a list of bytes into those bytes.
 *
 * Each byte is either decimal (`171`) or hexadecimal with a `0x` prefix (`0xAB`), from 0 to 255, without a sign. The
 * bytes are separated by spaces, commas or tabs. An empty text is an empty list.
 */
[[nodiscard]] std::expected<std::vector<uint8_t>, std::string> parseOctetString(std::string_view text);

/**
 * @brief Formats bytes the way parseOctetString() reads them (`0x01 0xAB`).
 */
[[nodiscard]] std::string formatOctetString(std::span<const uint8_t> bytes);

/**
 * @brief Pads a string with null bytes up to the size of its entry.
 *
 * @param length The `StringLengthMin` of the entry, which is the size of the string in the device.
 * @return The padded bytes, or an error when they do not fit.
 */
[[nodiscard]] std::expected<std::vector<uint8_t>, std::string> fitToLength(std::vector<uint8_t> bytes,
                                                                           std::size_t          length);

}    // namespace Frasy::CanOpen::OdValue

#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_OD_VALUE_H
