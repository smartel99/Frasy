/**
 * @file    Thread.h
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


#ifndef BRIGERAD_CORE_THREAD_H
#define BRIGERAD_CORE_THREAD_H

#ifdef _WIN32

#include <string_view>
#include <thread>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <processthreadsapi.h>

#if __has_include(<pthread.h>)
#include <pthread.h>
#endif

namespace Brigerad {
constexpr bool IsPthread()
{
    return !std::same_as<decltype(std::declval<std::jthread>().native_handle()), HANDLE>;
}
#if __has_include(<pthread.h>)
using ThreadHandle_t = pthread_t;
#else
using ThreadHandle_t = HANDLE;
#endif

bool SetThreadName(HANDLE thread, std::string_view name);
bool SetThreadPriority(HANDLE thread, int priority);
bool CancelSynchronousIo(ThreadHandle_t thread);
#if __has_include(<pthread.h>)
bool SetThreadName(pthread_t thread, std::string_view name);
bool SetThreadPriority(pthread_t thread, int priority);
#endif
}
#else
#error Only supported on Windows!
#endif
#endif //BRIGERAD_CORE_THREAD_H
