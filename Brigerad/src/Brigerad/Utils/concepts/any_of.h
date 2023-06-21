/**
 * @file    any_of.h
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

#ifndef BRIGERAD_UTILS_CONCEPTS_ANY_OF_H
#define BRIGERAD_UTILS_CONCEPTS_ANY_OF_H
#include <array>
#include <type_traits>
#include <vector>

namespace Brigerad
{
template<typename T, typename... Ts>
consteval bool AnyOf()
{
    return ((std::is_same_v<std::remove_cvref_t<T>, Ts>) || ...);
}

template<typename BaseType, typename... Expected>
struct IsAnyOf : std::conditional_t<AnyOf<BaseType, Expected...>(), std::true_type, std::false_type>
{
};

template<typename T>
struct IsArray : std::is_array<T>
{
};

template<typename T, size_t N>
struct IsArray<std::array<T, N>> : std::true_type
{
};

template<typename T>
struct IsArray<const T> : IsArray<T>
{
};

template<typename T>
struct IsArray<volatile T> : IsArray<T>
{
};

template<typename T>
struct IsArray<const volatile T> : IsArray<T>
{
};

template<typename T>
struct IsVector : std::false_type
{
};

template<typename T>
struct IsVector<std::vector<T>> : std::true_type
{
};

template<typename T>
struct IsVector<const T> : IsVector<T>
{
};

template<typename T>
struct IsVector<volatile T> : IsVector<T>
{
};

template<typename T>
struct IsVector<const volatile T> : IsVector<T>
{
};

}    // namespace Brigerad
#endif    // BRIGERAD_UTILS_CONCEPTS_ANY_OF_H
