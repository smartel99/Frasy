/**
 * @file    basic_serializer.h
 * @author  Samuel Martel
 * @date    2022-12-12
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

#ifndef BRIGERAD_UTILS_SERIALIZATION_BASIC_SERIALIZER_H
#define BRIGERAD_UTILS_SERIALIZATION_BASIC_SERIALIZER_H

#include "../concepts/constexpr_for.h"
#include "serializer.h"

#include <bit>
#include <boost/pfr.hpp>
#include <concepts>

namespace Brigerad
{
struct BasicSerializer
{
public:
    template<SerializeableContainer T>
    static std::vector<uint8_t> Serialize(const T& t)
    {
        constexpr size_t     bytesPerObj = sizeof(typename T::value_type);
        std::vector<uint8_t> serialized;
        serialized.reserve(t.size() * bytesPerObj);
        for (auto&& item : t)
        {
            auto tmp = Serialize(item);
            serialized.insert(serialized.end(), tmp.begin(), tmp.end());
        }

        return serialized;
    }

    template<typename T>
    static std::vector<uint8_t> Serialize(T&& t)
    {
        std::vector<uint8_t> serialized;

        boost::pfr::for_each_field(t,
                                   [&serialized](auto&& v)
                                   {
                                       auto tmp = std::bit_cast<std::array<uint8_t, sizeof(T)>>(v);
                                       if constexpr (std::endian::native == std::endian::big)
                                       {
                                           serialized.insert(serialized.end(), tmp.begin(), tmp.end());
                                       }
                                       else { serialized.insert(serialized.end(), tmp.rbegin(), tmp.rend()); }
                                   });

        return serialized;
    }

    template<typename T>
    static T Deserialize(const std::vector<uint8_t>& raw)
        requires(SerializeableContainer<T>())
    {
        return {raw.begin(), raw.end()};
    }

    template<Serializer<BasicSerializer> T>
    static T Deserialize(const std::vector<uint8_t>& raw)
    {
        T      t;
        size_t at = 0;

        boost::pfr::for_each_field(t,
                                   [&](auto&& field)
                                   {
                                       using FieldType                          = std::remove_cvref_t<decltype(field)>;
                                       constexpr size_t               fieldSize = sizeof(field);
                                       std::array<uint8_t, fieldSize> rawField  = [&]()
                                       {
                                           std::array<uint8_t, fieldSize> a;
                                           if constexpr (std::endian::native == std::endian::big)
                                           {
                                               for (auto&& e : a)
                                               {
                                                   e = raw[at];
                                                   at++;
                                               }
                                           }
                                           else
                                           {
                                               for (auto e = a.rbegin(); e != a.rend(); e++)
                                               {
                                                   *e = raw[at];
                                                   at++;
                                               }
                                           }
                                           return a;
                                       }();
                                       field = std::bit_cast<FieldType>(rawField);
                                   });

        return t;
    }
};
}    // namespace Brigerad

#endif    // BRIGERAD_UTILS_SERIALIZATION_BASIC_SERIALIZER_H
