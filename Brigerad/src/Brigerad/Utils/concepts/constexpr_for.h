/**
 * @file    constexpr_for.h
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

#ifndef BRIGERAD_UTILS_CONCEPTS_CONSTEXPR_FOR_H
#define BRIGERAD_UTILS_CONCEPTS_CONSTEXPR_FOR_H
#include <type_traits>

namespace Brigerad
{
template<auto Start, auto End, auto Inc, typename F>
constexpr void ConstexprFor(F&& f)
{
    if constexpr (Start < End)
    {
        f(std::integral_constant<decltype(Start), Start>());
        ConstexprFor<Start + Inc, End, Inc>(f);
    }
}


}    // namespace Brigerad
#endif    // BRIGERAD_UTILS_CONCEPTS_CONSTEXPR_FOR_H
