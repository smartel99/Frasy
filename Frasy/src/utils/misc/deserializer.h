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

#include "Brigerad/Core/Core.h"
#include "serializable_container.h"

#include <bit>
#include <boost/pfr.hpp>
#include <string>
#include <type_traits>
#include <vector>

namespace Frasy {
namespace Internal {
template<typename T>
concept Primitives = std::is_arithmetic_v<T> || std::same_as<T, std::string>;
}

template<typename T, typename Begin, typename End>
    requires((std::is_integral_v<T> && !std::is_same_v<T, bool>))
T Deserialize(Begin&& b, End&& e);

template<typename T, typename Begin, typename End>
    requires(std::same_as<T, float>)
float Deserialize(Begin&& b, End&& e);

template<typename T, typename Begin, typename End>
    requires(std::same_as<T, double>)
double Deserialize(Begin&& b, End&& e);

template<typename T, typename Begin, typename End>
    requires(std::same_as<T, bool>)
bool Deserialize(Begin&& b, End&& e);

template<SerializableContainer T, typename Begin, typename End>
T Deserialize(Begin&& b, End&& e);

template<typename T, typename Begin, typename End>
T Deserialize(Begin&& b, End&& e);


template<typename T, typename Begin, typename End>
    requires((std::is_integral_v<T> && !std::is_same_v<T, bool>))
T Deserialize(Begin&& b, End&& e)
{
    if (static_cast<long long int>(sizeof(T)) > std::distance(b, e)) { throw std::runtime_error("Not enough data!"); }
    constexpr size_t N = sizeof(T);
    T                t = {};
    for (size_t i = 1; i <= sizeof(T); i++) {
        t |= static_cast<T>(*b) << ((N * 8) - (i * 8));
        ++b;
    }
    return t;
}

template<typename T, typename Begin, typename End>
    requires(std::is_enum_v<T>)
T Deserialize(Begin&& b, End&& e)
{
    constexpr size_t          N = sizeof(T);
    std::underlying_type_t<T> t = {};
    for (size_t i = 1; i <= sizeof(T); i++) {
        t |= *b << ((N * 8) - (i * 8));
        ++b;
    }
    return static_cast<T>(t);
}

template<typename T, typename Begin, typename End>
    requires(std::same_as<T, float>)
float Deserialize(Begin&& b, End&& e)
{
    static_assert(sizeof(uint32_t) == sizeof(float));
    return std::bit_cast<float>(Deserialize<uint32_t>(b, e));
}

template<typename T, typename Begin, typename End>
    requires(std::same_as<T, double>)
double Deserialize(Begin&& b, End&& e)
{
    static_assert(sizeof(uint64_t) == sizeof(double));
    return std::bit_cast<double>(Deserialize<uint64_t>(b, e));
}

template<typename T, typename Begin, typename End>
    requires(std::same_as<T, bool>)
bool Deserialize(Begin&& b, End&& e)
{
    if (sizeof(bool) > std::distance(b, e)) { throw std::runtime_error("Not enough data!"); }
    bool v = (*b) == 0x01;
    b++;
    return v;
}

template<SerializableContainer T, typename Begin, typename End>
T Deserialize(Begin&& b, End&& e)
{
    if (std::distance(b, e) < static_cast<long long int>(sizeof(uint16_t))) {
        throw std::runtime_error("Not enough data");
    }
    T        t;
    uint16_t size = Deserialize<uint16_t>(b, e); // Get reported size from serializer
    if constexpr (std::is_same_v<std::array<typename T::value_type, sizeof(T) / sizeof(typename T::value_type)>, T>) {
        // Array, check size is valid
        if (size != sizeof(T) / sizeof(typename T::value_type)) {
            throw std::runtime_error("Received invalid size for array!");
        }
    }
    else if constexpr (std::is_same_v<std::vector<typename T::value_type>, T>) {
        // Vector, must resize
        t.resize(size);
    }
    else if constexpr (std::is_same_v<std::string, T>) {
        // String, must resize
        t.resize(size);
    }
    else { BR_CORE_ASSERT(false, "Invalid type for container deserialization"); }

    for (size_t i = 0; i < size; ++i) { t[i] = Deserialize<typename T::value_type>(b, e); }
    return t;
}

template<typename T, typename Begin, typename End>
T Deserialize(Begin&& b, End&& e)
{
    T t;
    boost::pfr::for_each_field(t, [&](auto&& i) { i = Deserialize<std::remove_cvref_t<decltype(i)>>(b, e); });
    return t;
}

} // namespace Frasy

//!@}

#endif    // GUARD_NILAI_SERVICES_DESERIALIZER_H
