/**
 * @file    vector_to_string.h
 * @author  Paul Thomas
 * @date    2023-02-16
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
#ifndef INSTRUMENTATION_BOARD_MAIN_UTILS_MISC_VECTOR_TO_STRING_H
#define INSTRUMENTATION_BOARD_MAIN_UTILS_MISC_VECTOR_TO_STRING_H

#include <array>
#include <sstream>
#include <string>
#include <vector>

namespace Frasy
{
template<typename T>
std::string VectorToString(std::vector<T> vector)
{
    std::stringstream ss;
    ss << "[";
    bool first = true;
    for (const auto& v : vector)
    {
        if (first) { first = false; }
        else { ss << ", "; }
        if constexpr (std::is_arithmetic_v<T>) { ss << std::to_string(v); }
        else if constexpr (std::is_enum_v<T>) { ss << std::to_string(static_cast<std::underlying_type_t<T>>(v)); }
        else { ss << std::string(v); }
    }
    ss << "]";
    return ss.str();
}

template<typename T, std::size_t N>
std::string ArrayToString(std::array<T, N> array)
{
    return VectorToString(std::vector<T> {array.begin(), array.end()});
}

}    // namespace Frasy

#endif    // INSTRUMENTATION_BOARD_MAIN_UTILS_MISC_VECTOR_TO_STRING_H
