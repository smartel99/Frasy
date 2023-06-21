/**
 * @file    string_literal.h
 * @author  Samuel Martel
 * @date    2023-04-17
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

#ifndef BRIGERAD_SRC_BRIGERAD_UTILS_TYPES_STRING_LITERAL_H
#define BRIGERAD_SRC_BRIGERAD_UTILS_TYPES_STRING_LITERAL_H

#include <algorithm>

namespace Brigerad
{
template<size_t N>
struct StringLiteral
{
    constexpr StringLiteral(const char(&str)[N]){
        std::copy_n(str, N, value);
    }

    char value[N];
};
}

#endif    // BRIGERAD_SRC_BRIGERAD_UTILS_TYPES_STRING_LITERAL_H
