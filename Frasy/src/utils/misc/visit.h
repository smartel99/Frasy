/**
 * @file    visit.h
 * @author  Samuel Martel
 * @date    2024-05-09
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


#ifndef FRASY_SRC_UTILS_MISC_VISIT_H
#define FRASY_SRC_UTILS_MISC_VISIT_H
#include <variant>

namespace Frasy {
namespace Details {
template<typename... Ts>
struct Visitors : Ts... {
    using Ts::operator()...;
};

template<typename... Ts>
Visitors(Ts...) -> Visitors<Ts...>;
}    // namespace Details

template<typename Variant, typename... Visitors>
inline constexpr auto visit(Variant&& variant, Visitors&&... visitors)
{
    return std::visit(Details::Visitors {std::forward<Visitors>(visitors)...}, std::forward<Variant>(variant));
}

}    // namespace Frasy

#endif    // FRASY_SRC_UTILS_MISC_VISIT_H
