/**
 * @file    ranges.h
 * @author  Samuel Martel
 * @date    2023-05-08
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

#ifndef GUARD_BRIGERAD_RANGES_H
#define GUARD_BRIGERAD_RANGES_H

#include <ranges>

namespace Brigerad
{
namespace Detail
{
// Type acts as a tag to find the correct operator| overload.
template<typename C>
struct ToHelper
{
};

// This actually does the work.
template<typename Container, std::ranges::range R>
    requires std::convertible_to<std::ranges::range_value_t<R>, typename Container::value_type>
constexpr Container operator|(R&& r, ToHelper<Container>)
{
    return Container {r.begin(), r.end()};
}

/**
 * Converts a proxy'd range into a container.
 * @tparam Container
 * @tparam R
 * @param r
 * @return
 */
template<typename Container, std::ranges::range R>
    requires std::constructible_from<Container,
                                     decltype(std::declval<std::ranges::range_value_t<R>>().begin()),
                                     decltype(std::declval<std::ranges::range_value_t<R>>().end())>
constexpr Container operator|(R&& r, ToHelper<Container>)
{
    Container c;
    c.reserve(std::distance(r.begin(), r.end()));
    for (auto&& v : r) { c.emplace_back(v.begin(), v.end()); }
    return c;
}
}    // namespace Detail

template<std::ranges::range Container>
requires(!std::ranges::view<Container>)
constexpr auto To()
{
    return Detail::ToHelper<Container>{};
}
}    // namespace Brigerad

#endif    // GUARD_BRIGERAD_RANGES_H
