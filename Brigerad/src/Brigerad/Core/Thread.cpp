/**
 * @file    Thread.cpp
 * @author  Sam Martel
 * @date    2026-02-14
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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "Thread.h"

#include <Brigerad/Core/Log.h>

#include <string>
#include <cstring>

namespace Brigerad {
namespace {
std::wstring utf8_to_wstring(std::string_view s)
{
    if (s.empty()) return {};

    int needed = ::MultiByteToWideChar(CP_UTF8,
                                       0,
                                       s.data(),
                                       static_cast<int>(s.size()),
                                       nullptr,
                                       0);
    if (needed <= 0) return {};

    std::wstring out(static_cast<size_t>(needed), L'\0');
    ::MultiByteToWideChar(CP_UTF8,
                          0,
                          s.data(),
                          static_cast<int>(s.size()),
                          out.data(),
                          needed);
    return out;
}
} // namespace

bool SetThreadName(HANDLE thread, std::string_view name)
{
    if (!thread) { return false; }

    const std::wstring wname = utf8_to_wstring(name);
    if (wname.empty() && !name.empty()) { return false; }

    return SUCCEEDED(SetThreadDescription(thread, wname.c_str()));
}

bool SetThreadPriority(HANDLE thread, int priority)
{
    return ::SetThreadPriority(thread, priority);
}

bool CancelSynchronousIo(ThreadHandle_t thread)
{
#if __has_include(<pthread.h>)
    BR_CORE_WARN("CancelSynchronousIo not implemented for POSIX!");
    return false;
#else
    return ::CancelSynchronousIo(thread) == 0;
#endif
}


#if __has_include(<pthread.h>)
bool SetThreadName(pthread_t thread, std::string_view name)
{
    // Many platforms limit to 16 bytes including null terminator (Linux).
    char buf[16]{};
    if (!name.empty()) {
        std::strncpy(&buf[0], name.data(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
    }

    return ::pthread_setname_np(thread, &buf[0]) == 0;
}

bool SetThreadPriority(pthread_t thread, int priority)
{
    return false; // lmao
}
#endif
} // namespace Brigerad
