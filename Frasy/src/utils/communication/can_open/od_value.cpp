/**
 * @file    od_value.cpp
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
#include "od_value.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <charconv>
#include <cstring>
#include <format>
#include <limits>
#include <type_traits>

namespace Frasy::CanOpen::OdValue {

namespace {
//! Raw data longer than this is truncated when displayed.
constexpr std::size_t s_maxDisplayedBytes = 64;

std::string hexBytes(std::span<const uint8_t> bytes)
{
    std::string out;
    const auto  shown = (std::min)(bytes.size(), s_maxDisplayedBytes);
    out.reserve(shown * 3 + 16);
    for (std::size_t i = 0; i < shown; ++i) {
        if (i != 0) { out += ' '; }
        out += std::format("{:02X}", bytes[i]);
    }
    if (bytes.size() > shown) { out += std::format(" ... ({} bytes)", bytes.size()); }
    return out;
}

template<typename T>
T fromBytes(std::span<const uint8_t> bytes)
{
    std::array<uint8_t, sizeof(T)> tmp {};
    std::copy_n(bytes.begin(), sizeof(T), tmp.begin());
    return std::bit_cast<T>(tmp);
}

template<typename T>
std::string formatNumber(std::span<const uint8_t> bytes, bool withHex)
{
    const T value = fromBytes<T>(bytes);
    if constexpr (std::is_floating_point_v<T>) { return std::format("{}", value); }
    else if constexpr (std::is_unsigned_v<T>) {
        if (!withHex) { return std::format("{}", value); }
        return std::format("{} (0x{:0{}X})", value, value, sizeof(T) * 2);
    }
    else {
        return std::format("{}", value);
    }
}

std::string formatString(std::span<const uint8_t> bytes)
{
    // Strings are not required to fill the whole object, the end is marked by a null character.
    const auto end = std::ranges::find(bytes, uint8_t {0});
    return {bytes.begin(), end};
}

template<typename T>
std::expected<std::vector<uint8_t>, std::string> toBytes(T value)
{
    std::vector<uint8_t> out(sizeof(T));
    std::memcpy(out.data(), &value, sizeof(T));
    return out;
}

std::string_view trim(std::string_view text)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    return text;
}

bool hasHexPrefix(std::string_view text)
{
    return text.size() >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X');
}

template<typename T>
std::expected<std::vector<uint8_t>, std::string> parseInteger(std::string_view text)
{
    constexpr bool isSigned = std::is_signed_v<T>;
    using Unsigned          = std::make_unsigned_t<T>;

    if (hasHexPrefix(text)) {
        // Hexadecimal is a bit pattern: 0xFF is a valid int8_t, equal to -1.
        Unsigned    value {};
        std::string_view digits = text.substr(2);
        auto [ptr, ec]          = std::from_chars(digits.data(), digits.data() + digits.size(), value, 16);
        if (digits.empty() || ptr != digits.data() + digits.size()) { return std::unexpected {"Invalid number"}; }
        if (ec == std::errc::result_out_of_range) { return std::unexpected {"Value out of range"}; }
        if (ec != std::errc {}) { return std::unexpected {"Invalid number"}; }
        return toBytes(static_cast<T>(value));
    }

    // from_chars does not accept a leading '+'.
    if (text.starts_with('+')) { text.remove_prefix(1); }
    T    value {};
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value, 10);
    if (ec == std::errc::result_out_of_range) { return std::unexpected {"Value out of range"}; }
    if (ec != std::errc {} || ptr != text.data() + text.size()) {
        if (!isSigned && text.starts_with('-')) { return std::unexpected {"Value out of range"}; }
        return std::unexpected {"Invalid number"};
    }
    return toBytes(value);
}

template<typename T>
std::expected<std::vector<uint8_t>, std::string> parseReal(std::string_view text)
{
    if (text.starts_with('+')) { text.remove_prefix(1); }
    T    value {};
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec == std::errc::result_out_of_range) { return std::unexpected {"Value out of range"}; }
    if (ec != std::errc {} || ptr != text.data() + text.size()) { return std::unexpected {"Invalid number"}; }
    return toBytes(value);
}

std::expected<std::vector<uint8_t>, std::string> parseBoolean(std::string_view text)
{
    std::string lower(text);
    std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower == "true" || lower == "1") { return std::vector<uint8_t> {1}; }
    if (lower == "false" || lower == "0") { return std::vector<uint8_t> {0}; }
    return std::unexpected {"Expected true, false, 1 or 0"};
}

std::expected<std::vector<uint8_t>, std::string> parseRaw(std::string_view text)
{
    constexpr std::string_view separators = " \t,";

    std::vector<uint8_t> out;
    std::size_t          pos = 0;
    while (pos < text.size()) {
        const auto start = text.find_first_not_of(separators, pos);
        if (start == std::string_view::npos) { break; }
        auto end = text.find_first_of(separators, start);
        if (end == std::string_view::npos) { end = text.size(); }
        auto token = text.substr(start, end - start);
        pos        = end;

        if (hasHexPrefix(token)) { token.remove_prefix(2); }
        // A single digit is one byte, a longer token is a run of bytes and needs complete pairs.
        if (token.empty() || (token.size() > 2 && token.size() % 2 != 0)) {
            return std::unexpected {"Hexadecimal data needs two digits per byte"};
        }
        const std::size_t step = token.size() == 1 ? 1 : 2;
        for (std::size_t i = 0; i < token.size(); i += step) {
            uint8_t byte = 0;
            auto [ptr, ec] = std::from_chars(token.data() + i, token.data() + i + step, byte, 16);
            if (ec != std::errc {} || ptr != token.data() + i + step) {
                return std::unexpected {"Invalid hexadecimal data"};
            }
            out.push_back(byte);
        }
    }
    if (out.empty()) { return std::unexpected {"No data"}; }
    return out;
}
}    // namespace

std::string_view toString(VarType type) noexcept
{
    switch (type) {
        case VarType::Undefined: return "raw";
        case VarType::Boolean: return "bool";
        case VarType::Signed8: return "int8_t";
        case VarType::Signed16: return "int16_t";
        case VarType::Signed32: return "int32_t";
        case VarType::Signed64: return "int64_t";
        case VarType::Unsigned8: return "uint8_t";
        case VarType::Unsigned16: return "uint16_t";
        case VarType::Unsigned32: return "uint32_t";
        case VarType::Unsigned64: return "uint64_t";
        case VarType::Real32: return "float";
        case VarType::Real64: return "double";
        case VarType::String: return "string";
        case VarType::Max:
        default: return "unknown";
    }
}

std::string format(VarType type, std::span<const uint8_t> bytes, bool withHex)
{
    if (type == VarType::String) { return formatString(bytes); }

    const auto expected = sizeOf(type);
    if (expected == 0) { return hexBytes(bytes); }
    if (bytes.size() != expected) {
        return std::format("{} [unexpected size: {} bytes, expected {}]", hexBytes(bytes), bytes.size(), expected);
    }

    switch (type) {
        case VarType::Boolean: return bytes[0] != 0 ? "true" : "false";
        case VarType::Signed8: return formatNumber<int8_t>(bytes, withHex);
        case VarType::Signed16: return formatNumber<int16_t>(bytes, withHex);
        case VarType::Signed32: return formatNumber<int32_t>(bytes, withHex);
        case VarType::Signed64: return formatNumber<int64_t>(bytes, withHex);
        case VarType::Unsigned8: return formatNumber<uint8_t>(bytes, withHex);
        case VarType::Unsigned16: return formatNumber<uint16_t>(bytes, withHex);
        case VarType::Unsigned32: return formatNumber<uint32_t>(bytes, withHex);
        case VarType::Unsigned64: return formatNumber<uint64_t>(bytes, withHex);
        case VarType::Real32: return formatNumber<float>(bytes, withHex);
        case VarType::Real64: return formatNumber<double>(bytes, withHex);
        default: return hexBytes(bytes);
    }
}

std::expected<std::vector<uint8_t>, std::string> parse(VarType type, std::string_view text)
{
    if (type == VarType::String) {
        // Same behavior as the SDO download tab: the terminating null character is part of the transfer.
        std::vector<uint8_t> out(text.begin(), text.end());
        out.push_back(0);
        return out;
    }

    text = trim(text);
    if (text.empty()) { return std::unexpected {"No value"}; }

    switch (type) {
        case VarType::Boolean: return parseBoolean(text);
        case VarType::Signed8: return parseInteger<int8_t>(text);
        case VarType::Signed16: return parseInteger<int16_t>(text);
        case VarType::Signed32: return parseInteger<int32_t>(text);
        case VarType::Signed64: return parseInteger<int64_t>(text);
        case VarType::Unsigned8: return parseInteger<uint8_t>(text);
        case VarType::Unsigned16: return parseInteger<uint16_t>(text);
        case VarType::Unsigned32: return parseInteger<uint32_t>(text);
        case VarType::Unsigned64: return parseInteger<uint64_t>(text);
        case VarType::Real32: return parseReal<float>(text);
        case VarType::Real64: return parseReal<double>(text);
        case VarType::Undefined: return parseRaw(text);
        case VarType::String:
        case VarType::Max:
        default: return std::unexpected {"Unsupported type"};
    }
}

}    // namespace Frasy::CanOpen::OdValue
