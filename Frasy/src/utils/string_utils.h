/**
 * @file    string_utils.h
 * @author  Samuel Martel
 * @date    2020-01-03
 * @brief   String helpers shared across Frasy: replacement, file paths, wide strings, trimming and case folding.
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this
 * program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef FRASY_UTILS_STRING_UTILS_H
#define FRASY_UTILS_STRING_UTILS_H

#include <string>
#include <string_view>

namespace Frasy::StringUtils {

std::string  replaceAll(const std::string& str, const std::string& toReplace, const std::string& replaceBy);
std::string  getFullNameFromPath(const std::wstring& path);
std::string  getFullNameFromPath(const std::string& path);
std::string  getNameFromPath(const std::string& path);
std::string  removeNameFromPath(const std::wstring& path);
std::string  removeNameFromPath(const std::string& path);
std::string  wstringToString(const std::wstring& src);
std::wstring stringToWstring(const std::string& src);

//! Removes the leading and trailing whitespace of @p text.
[[nodiscard]] std::string_view trim(std::string_view text);
//! Copy of @p text with its ASCII letters in lowercase.
[[nodiscard]] std::string toLower(std::string_view text);
//! True when @p text starts with "0x" or "0X".
[[nodiscard]] bool hasHexPrefix(std::string_view text);

}    // namespace Frasy::StringUtils

#endif    // FRASY_UTILS_STRING_UTILS_H
