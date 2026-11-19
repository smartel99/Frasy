/**
 * @file    string_utils.cpp
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
#include "string_utils.h"

#include <Windows.h>

#include <algorithm>
#include <cctype>

namespace Frasy::StringUtils {

std::string replaceAll(const std::string& str, const std::string& toReplace, const std::string& replaceBy)
{
    std::string newStr = str;

    std::string::size_type n = 0;
    while ((n = newStr.find(toReplace, n)) != std::string::npos) {
        newStr.replace(n, toReplace.size(), replaceBy);
        n += replaceBy.size();
    }

    return newStr;
}

std::string getFullNameFromPath(const std::wstring& path)
{
    std::string strTo       = wstringToString(path);
    size_t      startOfName = strTo.find_last_of("/\\");
    strTo                   = strTo.substr(startOfName + 1);
    return strTo;
}

std::string getFullNameFromPath(const std::string& path)
{
    std::string strTo       = path;
    size_t      startOfName = strTo.find_last_of("/\\");
    strTo                   = strTo.substr(startOfName + 1);
    return strTo;
}

std::string getNameFromPath(const std::string& path)
{
    std::string strTo      = getFullNameFromPath(path);
    size_t      startOfExt = strTo.find_last_of(".");
    return strTo.substr(0, startOfExt);
}

std::string removeNameFromPath(const std::wstring& path)
{
    std::string strTo       = wstringToString(path);
    size_t      startOfName = strTo.find_last_of("/\\");
    strTo                   = strTo.substr(0, strTo.size() - startOfName);
    return strTo;
}

std::string removeNameFromPath(const std::string& path)
{
    std::string strTo       = path;
    size_t      startOfName = strTo.find_last_of("/\\");
    strTo                   = strTo.substr(0, startOfName + 1);
    return strTo;
}

std::string wstringToString(const std::wstring& src)
{
    std::string strTo;
    char*       szTo = new char[src.length() + 1];
    szTo[src.size()] = '\0';
    WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, szTo, (int)src.length(), nullptr, nullptr);
    strTo = szTo;
    delete[] szTo;

    return strTo;
}

std::wstring stringToWstring(const std::string& src)
{ return std::wstring(src.begin(), src.end()); }

std::string_view trim(std::string_view text)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    return text;
}

std::string toLower(std::string_view text)
{
    std::string out(text);
    std::ranges::transform(out, out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool hasHexPrefix(std::string_view text)
{ return text.size() >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'); }

}    // namespace Frasy::StringUtils
