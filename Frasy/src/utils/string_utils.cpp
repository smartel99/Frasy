#include "string_utils.h"

#include <Windows.h>

namespace Frasy::StringUtils
{
std::string ReplaceAll(const std::string& str,
                       const std::string& toReplace,
                       const std::string& replaceBy)
{
    std::string newStr = str;

    std::string::size_type n = 0;
    while ((n = newStr.find(toReplace, n)) != std::string::npos)
    {
        newStr.replace(n, toReplace.size(), replaceBy);
        n += replaceBy.size();
    }

    return newStr;
}

std::string GetFullNameFromPath(const std::wstring& path)
{
    std::string strTo       = WStringToString(path);
    size_t      startOfName = strTo.find_last_of("/\\");
    strTo                   = strTo.substr(startOfName + 1);
    return strTo;
}

std::string GetFullNameFromPath(const std::string& path)
{
    std::string strTo       = path;
    size_t      startOfName = strTo.find_last_of("/\\");
    strTo                   = strTo.substr(startOfName + 1);
    return strTo;
}

std::string GetNameFromPath(const std::string& path)
{
    std::string strTo      = GetFullNameFromPath(path);
    size_t      startOfExt = strTo.find_last_of(".");
    return strTo.substr(0, startOfExt);
}

std::string RemoveNameFromPath(const std::wstring& path)
{
    std::string strTo       = WStringToString(path);
    size_t      startOfName = strTo.find_last_of("/\\");
    strTo                   = strTo.substr(0, strTo.size() - startOfName);
    return strTo;
}

std::string RemoveNameFromPath(const std::string& path)
{
    std::string strTo       = path;
    size_t      startOfName = strTo.find_last_of("/\\");
    strTo                   = strTo.substr(0, startOfName + 1);
    return strTo;
}

std::string WStringToString(const std::wstring& src)
{
    std::string strTo;
    char*       szTo = new char[src.length() + 1];
    szTo[src.size()] = '\0';
    WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, szTo, (int)src.length(), nullptr, nullptr);
    strTo = szTo;
    delete[] szTo;

    return strTo;
}

std::wstring StringToWString(const std::string& src)
{
    return std::wstring(src.begin(), src.end());
}
}    // namespace Frasy::StringUtils
