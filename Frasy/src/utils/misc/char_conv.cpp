/**
 * @file    char_conv.cpp
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
#include "char_conv.h"

#include <array>
#include <Brigerad/Utils/concepts/constexpr_for.h>

namespace Frasy::Internal
{
template<typename T, size_t N = sizeof(T) * 2>
consteval std::array<uint8_t, N> MakeArray(uint8_t c)
{
    std::array<uint8_t, N> a = {};
    for (auto& v : a) { v = c; }
    return a;
}

template<typename T>
consteval T MakeValue(uint8_t c)
{
    T t = 0;
    for (size_t i = 0; i < sizeof(T) * 2; i++) { t |= static_cast<T>(static_cast<T>(c) << (i * 4)); }
    return t;
}

template<typename T>
consteval auto MakePair(uint8_t c, uint8_t v)
{
    return std::pair {MakeArray<T>(c), MakeValue<T>(v)};
}


template<typename T>
consteval bool ValidateAsciiToT()
{
#define MAKE_VALID_PAIR(v)   MakePair<T>((#v)[0], 0x##v)
#define MAKE_INVALID_PAIR(v) MakePair<T>(v, 0)
    constexpr std::array validTests = {
      MAKE_VALID_PAIR(0), MAKE_VALID_PAIR(1), MAKE_VALID_PAIR(2), MAKE_VALID_PAIR(3), MAKE_VALID_PAIR(4),
      MAKE_VALID_PAIR(5), MAKE_VALID_PAIR(6), MAKE_VALID_PAIR(7), MAKE_VALID_PAIR(8), MAKE_VALID_PAIR(9),
      MAKE_VALID_PAIR(a), MAKE_VALID_PAIR(b), MAKE_VALID_PAIR(c), MAKE_VALID_PAIR(d), MAKE_VALID_PAIR(e),
      MAKE_VALID_PAIR(f), MAKE_VALID_PAIR(A), MAKE_VALID_PAIR(B), MAKE_VALID_PAIR(C), MAKE_VALID_PAIR(D),
      MAKE_VALID_PAIR(E), MAKE_VALID_PAIR(F),
    };
    constexpr std::array invalidTests = {
      MAKE_INVALID_PAIR(0),
      MAKE_INVALID_PAIR(1),
      MAKE_INVALID_PAIR(2),
      MAKE_INVALID_PAIR(3),
      MAKE_INVALID_PAIR(4),
      MAKE_INVALID_PAIR(5),
      MAKE_INVALID_PAIR(6),
      MAKE_INVALID_PAIR(7),
      MAKE_INVALID_PAIR(8),
      MAKE_INVALID_PAIR(9),
      MAKE_INVALID_PAIR(' '),
    };

    Brigerad::ConstexprFor<0, validTests.size(), 1>(
      [&validTests](auto i) { static_assert(AsciiToT<T>(validTests[i].first.data()) == validTests[i].second); });

    Brigerad::ConstexprFor<0, invalidTests.size(), 1>(
      [&invalidTests](auto i) { static_assert(AsciiToT<T>(invalidTests[i].first.data()) == invalidTests[i].second); });

    return true;
#undef MAKE_VALID_PAIR
#undef MAKE_INVALID_PAIR
}
static_assert(ValidateAsciiToT<bool>());
static_assert(ValidateAsciiToT<uint8_t>());
static_assert(ValidateAsciiToT<int8_t>());
static_assert(ValidateAsciiToT<uint16_t>());
static_assert(ValidateAsciiToT<int16_t>());
static_assert(ValidateAsciiToT<uint32_t>());
static_assert(ValidateAsciiToT<int32_t>());
static_assert(ValidateAsciiToT<uint64_t>());
static_assert(ValidateAsciiToT<int64_t>());

template<typename T>
consteval bool ValidateTToAscii()
{
#define MAKE_VALID_PAIR(v)   MakePair<T>((#v)[0], 0x##v)
#define MAKE_INVALID_PAIR(v) MakePair<T>(v, 0)
    constexpr std::array validTests = {
      MAKE_VALID_PAIR(0), MAKE_VALID_PAIR(1), MAKE_VALID_PAIR(2), MAKE_VALID_PAIR(3), MAKE_VALID_PAIR(4),
      MAKE_VALID_PAIR(5), MAKE_VALID_PAIR(6), MAKE_VALID_PAIR(7), MAKE_VALID_PAIR(8), MAKE_VALID_PAIR(9),
      MAKE_VALID_PAIR(a), MAKE_VALID_PAIR(b), MAKE_VALID_PAIR(c), MAKE_VALID_PAIR(d), MAKE_VALID_PAIR(e),
      MAKE_VALID_PAIR(f), MAKE_VALID_PAIR(A), MAKE_VALID_PAIR(B), MAKE_VALID_PAIR(C), MAKE_VALID_PAIR(D),
      MAKE_VALID_PAIR(E), MAKE_VALID_PAIR(F),
    };
    constexpr std::array invalidTests = {
      MAKE_INVALID_PAIR(0),
      MAKE_INVALID_PAIR(1),
      MAKE_INVALID_PAIR(2),
      MAKE_INVALID_PAIR(3),
      MAKE_INVALID_PAIR(4),
      MAKE_INVALID_PAIR(5),
      MAKE_INVALID_PAIR(6),
      MAKE_INVALID_PAIR(7),
      MAKE_INVALID_PAIR(8),
      MAKE_INVALID_PAIR(9),
      MAKE_INVALID_PAIR(' '),
    };

    Brigerad::ConstexprFor<0, validTests.size(), 1>(
      [&validTests](auto i) { static_assert(TToAscii<T>(validTests[i].second) == validTests[i].first); });

    Brigerad::ConstexprFor<0, invalidTests.size(), 1>(
      [&invalidTests](auto i) { static_assert(TToAscii<T>(invalidTests[i].second) == invalidTests[i].first); });

    return true;
#undef MAKE_VALID_PAIR
#undef MAKE_INVALID_PAIR
}
// TODO Fix ValidateTToAscii.
// static_assert(ValidateTToAscii<bool>());
// static_assert(ValidateTToAscii<uint8_t>());
// static_assert(ValidateTToAscii<int8_t>());
// static_assert(ValidateTToAscii<uint16_t>());
// static_assert(ValidateTToAscii<int16_t>());
// static_assert(ValidateTToAscii<uint32_t>());
// static_assert(ValidateTToAscii<int32_t>());
// static_assert(ValidateTToAscii<uint64_t>());
// static_assert(ValidateTToAscii<int64_t>());
}    // namespace Frasy::Internal
