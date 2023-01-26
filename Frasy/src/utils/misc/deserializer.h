/**
 * @file    deserializer.h
 * @author  Samuel Martel
 * @date    2022-10-11
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

#ifndef GUARD_NILAI_SERVICES_DESERIALIZER_H
#define GUARD_NILAI_SERVICES_DESERIALIZER_H


#include <boost/pfr.hpp>
#include <Brigerad/Core/Core.h>
#include <Brigerad/Utils/concepts/is_array_like.h>
#include <string>
#include <type_traits>
#include <vector>

namespace Frasy
{
namespace Internal
{
template<typename T>
concept Primitives = std::is_arithmetic_v<T> || std::same_as<T, std::string>;
}

template<Internal::Primitives T, uint8_t... Is>
T Deserialize(const std::vector<uint8_t>& v, [[maybe_unused]] const std::integer_sequence<uint8_t, Is...>& is)
{
    return Pack<T>(v[Is]...);
}


template<typename Begin, typename End>
float Deserialize(Begin b, End e)
{
    static_assert(sizeof(uint32_t) == sizeof(float));
    return std::bit_cast<float>(Deserialize<uint32_t>(b, e));
}

template<typename Begin, typename End>
double Deserialize(Begin b, End e)
{
    static_assert(sizeof(uint64_t) == sizeof(double));
    return std::bit_cast<double>(Deserialize<uint64_t>(b, e));
}

template<std::integral T, typename Begin, typename End>
T Deserialize(Begin b, End e)
{
    constexpr size_t N = sizeof(T);
    T                t = {};
    BR_CORE_ASSERT(N <= std::distance(b, e), "Not enough data! Needs {} bytes, only have {}", N, std::distance(b, e));
    for (size_t i = 1; i <= sizeof(T); i++)
    {
        t |= static_cast<T>(*b) << ((N * 8) - (i * 8));
        ++b;
    }
    return t;
}

template<Brigerad::array_like T, typename Begin, typename End>
T Deserialize(Begin b, End e)
{
    T t;
    for (auto&& i : t)
    {
        using I = std::remove_cvref_t<decltype(i)>;
        BR_CORE_ASSERT(b != e, "reached the end!");
        i = Deserialize<I>(b, e);
        b += sizeof(I);
    }
    return t;
}

template<typename T, typename Begin, typename End>
T Deserialize(Begin b, End e)
{
    T t;
    boost::pfr::for_each_field(t,
                               [&](auto&& i)
                               {
                                   using I = std::remove_cvref_t<decltype(i)>;
                                   BR_CORE_ASSERT(b != e, "reached the end!");
                                   i = Deserialize<std::remove_cvref_t<decltype(i)>>(b, e);
                                   b += sizeof(I);
                               });
    return t;
}
}    // namespace Frasy

//!@}

#endif    // GUARD_NILAI_SERVICES_DESERIALIZER_H
