/**
 * @file    is_array_like.h
 * @author  Samuel Martel
 * @date    2022-12-14
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

#ifndef BRIGERAD_CONCEPTS_IS_ARRAY_LIKE_H
#define BRIGERAD_CONCEPTS_IS_ARRAY_LIKE_H
#include <array>
#include <type_traits>

namespace Brigerad
{
template<class T>
struct is_array_like : std::is_array<T>
{
};

template<class T, std::size_t N>
struct is_array_like<std::array<T, N>> : std::true_type
{
};

template<typename T>
concept array_like = is_array_like<T>::value;
}    // namespace Brigerad
#endif    // BRIGERAD_CONCEPTS_IS_ARRAY_LIKE_H
