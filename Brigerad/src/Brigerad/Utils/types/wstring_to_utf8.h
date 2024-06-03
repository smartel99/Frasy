/**
 * @file    wstring_to_utf8.h
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

#ifndef WSTRING_TO_UTF8_H
#define WSTRING_TO_UTF8_H

#include <string>

std::string wstring_to_utf8(const std::wstring& wstr);

#endif //WSTRING_TO_UTF8_H
