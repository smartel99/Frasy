/**
 * @file    concepts.h
 * @author  Paul Thomas
 * @date    2023-03-01
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
#ifndef BRIGERAD_FRASY_SRC_UTILS_CONCEPTS_H
#define BRIGERAD_FRASY_SRC_UTILS_CONCEPTS_H


#include <type_traits>

namespace Frasy
{
template<typename T>
concept Enum = std::is_enum_v<T>;

template<typename T>
concept IntegralAndNotBool = std::is_integral_v<T> && !std::is_same_v<T, bool>;

}    // namespace Frasy

#endif    // BRIGERAD_FRASY_SRC_UTILS_CONCEPTS_H
