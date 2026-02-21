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
#include <fstream>
#include <utility>

#include <cpptrace/from_current.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <processthreadsapi.h>

#if __has_include(<pthread.h>)
#include <pthread.h>
#endif

#define BR_BEGIN_GUARDED_SCOPE \
[&]{      \
    CPPTRACE_SEH_TRY {\
        [&] {\
            CPPTRACE_TRY

#define BR_END_GUARDED_SCOPE \
             CPPTRACE_CATCH(std::exception& e) {\
                ::Brigerad::_internalDoNotUse::addExceptionMessage(e);\
                ::Brigerad::_internalDoNotUse::printException(cpptrace::from_current_exception());\
            }\
        }();\
    }\
CPPTRACE_SEH_EXCEPT(::Brigerad::_internalDoNotUse::parseFilter(GetExceptionCode())) {\
                    ::Brigerad::_internalDoNotUse::printException(cpptrace::from_current_exception());\
    } \
}();

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

ThreadHandle_t GetCurrentThread();

namespace _internalDoNotUse {
bool initExceptionHandling();
void addExceptionMessage(const std::exception& e);

int  parseFilter(unsigned long code);
void printException(const cpptrace::stacktrace& trace);
}

template<typename... Args>
std::jthread MakeThread(auto&& f, Args&&... args)
{
    // If f takes a stop_token:
    if constexpr (std::is_invocable_v<decltype(f), std::stop_token, Args...>) {
        return std::jthread([&](std::stop_token st) {
            BR_BEGIN_GUARDED_SCOPE
                {
                    std::invoke(std::forward<decltype(f)>(f),
                                st,
                                std::forward<Args>(args)...);
                }
            BR_END_GUARDED_SCOPE
        });
    }
    else {
        return std::jthread([&] {
            BR_BEGIN_GUARDED_SCOPE
                {
                    std::invoke(std::forward<decltype(f)>(f),
                                std::forward<Args>(args)...);
                }
            BR_END_GUARDED_SCOPE
        });
    }
}

#if __has_include(<pthread.h>)
bool SetThreadName(pthread_t thread, std::string_view name);
bool SetThreadPriority(pthread_t thread, int priority);
#endif
}
#else
#error Only supported on Windows!
#endif
#endif //BRIGERAD_CORE_THREAD_H
