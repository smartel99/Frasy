/**
 * @file    members_are_only.h
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

#ifndef BRIGERAD_UTILS_CONCEPTS_MEMBERS_ARE_ONLY_H
#define BRIGERAD_UTILS_CONCEPTS_MEMBERS_ARE_ONLY_H

#include "any_of.h"
#include "constexpr_for.h"

#include <boost/pfr.hpp>
#include <type_traits>

namespace Brigerad
{
namespace Impl
{
template<size_t Index, typename T>
struct integer_sequence_sum;

template<size_t Index, size_t... Rhs>
struct integer_sequence_sum<Index, std::integer_sequence<size_t, Rhs...>>
{
    using type = std::integer_sequence<size_t, Index, Rhs...>;
};

template<typename T>
struct is_integer_sequence : std::false_type
{
};

template<typename T, T... t>
struct is_integer_sequence<std::integer_sequence<T, t...>> : std::true_type
{
};

template<typename Concept, typename T, size_t Index = 0>
constexpr auto first_member_failing()
{
    using BareT = std::remove_cvref_t<T>;
    static_assert(std::is_aggregate_v<BareT> || std::is_scalar_v<BareT>, "Type must be simple aggregate!");

    if constexpr (Index < boost::pfr::tuple_size_v<BareT>)
    {
        using IndexedType = boost::pfr::tuple_element_t<Index, BareT>;
        using ApplyRet    = decltype(Concept::template Apply<IndexedType>());
        if constexpr (!std::same_as<ApplyRet, void>)
        {
            // Found an error!
            struct ErrInfo
            {
                // Not using IndexedType to be able to print the actual type.
                using BadType = boost::pfr::tuple_element_t<Index, BareT>;
                using Path    = integer_sequence_sum<Index, typename ApplyRet::Path>::type;
            };
            return ErrInfo {};
        }
        else
        {
            // Iterate to the next element.
            return first_member_failing<Concept, BareT, Index + 1>();
        }
    }
    else
    {
        // We're done iterating, everything is good.
        return;
    }
}

template<template<typename> typename Compare,
         bool AllowStrings = false,
         bool AllowArrays  = false,
         bool AllowVectors = false>
struct check_recursively
{
    template<typename T>
    static constexpr auto Apply()
    {
        struct Err
        {
            using Type = T;
            using Path = std::integer_sequence<size_t>;
        };
        auto ApplyCheck = []()
        {
            if constexpr (!Compare<T>::value) { return Err {}; }
        };

        if constexpr (AllowStrings && std::same_as<T, std::string>) { return ApplyCheck(); }
        else if constexpr (AllowArrays && IsArray<T>::value) { return ApplyCheck(); }
        else if constexpr (AllowVectors && IsVector<T>::value) { return ApplyCheck(); }
        else if constexpr (std::is_class_v<T>)
        {
            // Recurse if T is a class.
            return first_member_failing<check_recursively<Compare, AllowStrings, AllowArrays, AllowVectors>, T>();
        }
        else { return ApplyCheck(); }
    }
};

template<typename T, template<typename> typename Check>
struct check_all_members
{
private:
    static consteval bool Apply()
    {
        using F = decltype(first_member_failing<check_recursively<Check>, T>());

        if constexpr (std::same_as<decltype(std::declval<F>()), void>) { return true; }
        else
        {
            MakeError<T>(decltype(std::declval<F::BadType>()) {}, decltype(std::declval<F::Path>()) {});
            MakeError<T>(typename F::BadType {}, decltype(std::declval<F::Path>()) {});
            return false;
        }
    }

    template<typename BaseType>
    static consteval void MakeError(auto invalidType, auto pathToMember)
    {
        static_assert(pathToMember.size() == 0);
    }

public:
    static constexpr bool value = Apply();
};
}    // namespace Impl

template<typename T, template<typename> typename Check>
concept MembersAllMeetCheck = Impl::check_all_members<std::remove_cvref_t<T>, Check>::value;

}    // namespace Brigerad

#endif    // BRIGERAD_UTILS_CONCEPTS_MEMBERS_ARE_ONLY_H
