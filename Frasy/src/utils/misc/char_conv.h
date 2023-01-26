/**
 * @file    char_conv.h
 * @author  Samuel Martel
 * @date    2022-12-08
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

#ifndef FRASY_UTILS_MISC_CHAR_CONV_H
#define FRASY_UTILS_MISC_CHAR_CONV_H

#include "type_size.h"

#include <array>
#include <charconv>
#include <concepts>
#include <cstdint>
#include <system_error>
#include <utility>


namespace Frasy
{
template<std::integral T>
[[nodiscard]] constexpr T AsciiToT(const uint8_t* data)
{
    T      v         = {};
    size_t charsForT = sizeof(T) * 2;
    for (size_t i = 0; i < charsForT; i++)
    {
        const uint8_t& current = data[i];
        uint8_t        value   = 0;
        if ((current >= 'a') && (current <= 'f')) { value = (current - 'a') + 0xA; }
        else if ((current >= 'A') && (current <= 'F')) { value = (current - 'A') + 0xA; }
        else if ((current >= '0') && (current <= '9')) { value = current - '0'; }

        constexpr size_t bitsPerChar = 4;
        size_t           shiftBy     = ((charsForT * bitsPerChar) - bitsPerChar) - (i * bitsPerChar);
        v |= static_cast<T>(static_cast<T>(value) << shiftBy);
    }

    return v;
}

template<std::integral T, size_t CharsForT = sizeof(T) * 2>
[[nodiscard]] constexpr std::array<uint8_t, CharsForT> TToAscii(T t)
{
    std::array<uint8_t, CharsForT> out = {};

    for (int i = CharsForT - 1; i >= 0; i--)
    {
        uint8_t v = static_cast<uint8_t>(t) & 0xF;
        if ((v >= 0) && (v <= 9)) { v += '0'; }
        else if ((v >= 0xA) && (v <= 0xF)) { v = (v - 0xA) + 'A'; }
        out[i] = v;

        t = static_cast<T>(t >> 4);
    }
    return out;
}

template<typename T>
consteval size_t SizeInChars([[maybe_unused]] T t = {})
{
    size_t charsPerByte = ArraySize<decltype(TToAscii<uint8_t>(0))>::value;
    return charsPerByte * sizeof(T);
}

}    // namespace Frasy

#endif    // FRASY_UTILS_MISC_CHAR_CONV_H
