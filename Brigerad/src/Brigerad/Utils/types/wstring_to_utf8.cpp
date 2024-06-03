/**
 * @file    wstring_to_utf8.cpp
 * @author  Paul Thomas
 * @date    6/3/2024
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version. This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details. You should have received a copy of the GNU General Public License
 * along with this program. If not, see <a
 * href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#include "wstring_to_utf8.h"

#include <locale>

std::string wstring_to_utf8(const std::wstring& wstr)
{
    using converter_type        = std::codecvt<wchar_t, char, std::mbstate_t>;
    const std::locale my_locale = std::locale("en_US.utf8");

    const converter_type& my_converter = std::use_facet<converter_type>(my_locale);

    std::mbstate_t state {};

    std::string result(wstr.size() * my_converter.max_length(), '\0');

    const wchar_t* from_next;
    char*          to_next;

    my_converter.out(
      state, wstr.data(), wstr.data() + wstr.size(), from_next, result.data(), result.data() + result.size(), to_next);

    result.resize(to_next - result.data());

    return result;
}