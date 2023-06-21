/**
 * @file    serializer.h
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

#ifndef BRIGERAD_UTILS_SERIALIZATION_SERIALIZER_H
#define BRIGERAD_UTILS_SERIALIZATION_SERIALIZER_H

#include "../concepts/members_all_match.h"

#include <array>
#include <concepts>
#include <string>
#include <vector>

namespace Brigerad
{
namespace Impl
{
template<typename T, typename U>
concept CanSerializeToArray = requires(U u) {
                                  {
                                      T::template Serialize<U>(u)
                                  } -> std::same_as<std::array<uint8_t, sizeof(U)>>;
                              };

template<typename T, typename U>
concept CanSerializeToVector = requires(U u) {
                                   {
                                       T::template Serialize<U>(u)
                                   } -> std::same_as<std::vector<uint8_t>>;
                               };

template<typename T, typename U>
concept CanSerializeToString = requires(U u) {
                                   {
                                       T::template Serialize<U>(u)
                                   } -> std::same_as<std::string>;
                               };

template<typename T, typename U>
concept CanSerialize = CanSerializeToArray<T, U> || CanSerializeToVector<T, U> || CanSerializeToString<T, U>;

template<typename T, typename U>
concept CanDeserializeFromArray = requires {
                                      {
                                          T::template Deserialize<U>(std::array<uint8_t, sizeof(U)> {})
                                      } -> std::same_as<U>;
                                  };

template<typename T, typename U>
concept CanDeserializeFromVector = requires {
                                       {
                                           T::template Deserialize<U>(std::vector<uint8_t> {})
                                       } -> std::same_as<U>;
                                   };

template<typename T, typename U>
concept CanDeserializeFromString = requires {
                                       {
                                           T::template Deserialize<U>(std::string {})
                                       } -> std::same_as<U>;
                                   };

template<typename T, typename U>
concept CanDeserialize =
  CanDeserializeFromArray<T, U> || CanDeserializeFromVector<T, U> || CanDeserializeFromString<T, U>;


template<typename Serializer>
struct SerializerAcceptsT
{
    template<typename T>
    struct Accepts : std::conditional_t<Impl::CanSerialize<Serializer, T> && Impl::CanDeserialize<Serializer, T>,
                                        std::true_type,
                                        std::false_type>
    {
    };
};
}    // namespace Impl

template<typename T>
concept SerializeableContainer = requires(T t) {
                                     typename T::value_type;

                                     typename T::iterator;
                                     typename T::const_iterator;
                                     {
                                         t.begin()
                                     } -> std::same_as<typename T::iterator>;
                                     {
                                         std::as_const(t).begin()
                                     } -> std::same_as<typename T::const_iterator>;

                                     {
                                         t.end()
                                     } -> std::same_as<typename T::iterator>;
                                     {
                                         std::as_const(t).end()
                                     } -> std::same_as<typename T::const_iterator>;

                                     {
                                         t.size()
                                     } -> std::same_as<typename T::size_type>;
                                     {
                                         std::as_const(t).size()
                                     } -> std::same_as<typename T::size_type>;
                                 } && std::is_arithmetic_v<typename T::value_type>;


template<typename S, typename T>
concept Serializer = MembersAllMeetCheck<T, Impl::SerializerAcceptsT<S>::template Accepts>;


}    // namespace Brigerad

#endif    // BRIGERAD_UTILS_SERIALIZATION_SERIALIZER_H
