/**
 * @file    serializer.h
 * @author  Samuel Martel
 * @date    2022-09-22
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

#ifndef GUARD_NILAI_SERVICES_SERIALIZER_H
#define GUARD_NILAI_SERVICES_SERIALIZER_H

#include "serializable_container.h"
#include <algorithm>
#include <array>
#include <bit>
#include <boost/pfr.hpp>
#include <string>
#include <type_traits>
#include <vector>

namespace Frasy
{
/**
 * A type that can be serialized.
 * @tparam T
 */
template<typename T>
concept Serializable =
  std::is_arithmetic_v<T> || SerializableContainer<T> || std::convertible_to<T, std::vector<uint8_t>>;

template<typename T>
std::vector<uint8_t> Serialize(const T& t)
{
    if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
    {
        auto tAsBytes = std::bit_cast<std::array<uint8_t, sizeof(T)>>(t);
        if constexpr (std::endian::native == std::endian::little) { std::reverse(tAsBytes.begin(), tAsBytes.end()); }
        return std::vector<uint8_t> {tAsBytes.begin(), tAsBytes.end()};
    }
    else if constexpr (SerializableContainer<T>)
    {
        std::vector<uint8_t> out;
        std::vector<uint8_t> size = Serialize(static_cast<uint16_t>(t.size()));
        out.insert(out.end(), size.begin(), size.end());
        for (auto&& v : t)
        {
            auto serV = Serialize(v);
            out.insert(out.end(), serV.begin(), serV.end());
        }
        return out;
    }
    else
    {
        std::vector<uint8_t> out;
        boost::pfr::for_each_field(t,
                                   [&](auto&& i)
                                   {
                                       auto tmp = Serialize(i);
                                       out.insert(out.end(), tmp.begin(), tmp.end());
                                   });
        return out;
    }
}

}    // namespace Frasy

//@}
#endif    // GUARD_NILAI_SERVICES_SERIALIZER_H
