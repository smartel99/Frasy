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
#include <Brigerad/Utils/dialogs/error.h>

#include <cpptrace/formatting.hpp>

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

namespace _internalDoNotUse {
static constexpr std::size_t      s_crashReportHeaderMaxSize = 1024;
static constexpr std::string_view s_stacktraceHeader         = "\nStack trace (most recent call first):\n";
static constexpr std::size_t      s_crashReportContentSize   = s_crashReportHeaderMaxSize - s_stacktraceHeader.size();
inline std::string                crashReportContent;
inline std::string                crashReportHeader;
inline std::ofstream              crashReportFile;

bool initExceptionHandling()
{
    crashReportFile = std::ofstream{"crash_report.txt"};
    BR_CORE_ASSERT(crashReportFile.is_open(), "Unable to open crash report file!");
    // Reserve memory for the string so that we are guaranteed to have the space for it, even in the case we fucked the dog way too hard
    crashReportHeader.resize(s_crashReportHeaderMaxSize, 0);
    crashReportContent.resize(s_crashReportContentSize, 0);

    return true;
}

void addExceptionMessage(const std::exception& e)
{
    std::format_to_n(crashReportContent.begin(),
                     s_crashReportContentSize,
                     "Unhandled exception: {}",
                     e.what());
}

int parseFilter(unsigned long code)
{
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION: std::format_to_n(crashReportContent.begin(),
                                                          s_crashReportContentSize,
                                                          "Access Violation");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: std::format_to_n(crashReportContent.begin(),
                                                               s_crashReportContentSize,
                                                               "Array Bounds Exceeded");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_BREAKPOINT: std::format_to_n(crashReportContent.begin(),
                                                    s_crashReportContentSize,
                                                    "Breakpoint Hit");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_DATATYPE_MISALIGNMENT: std::format_to_n(crashReportContent.begin(),
                                                               s_crashReportContentSize,
                                                               "Datatype Misalignment");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_FLT_DENORMAL_OPERAND: std::format_to_n(crashReportContent.begin(),
                                                              s_crashReportContentSize,
                                                              "Floating Point Denormal Operand");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO: std::format_to_n(crashReportContent.begin(),
                                                            s_crashReportContentSize,
                                                            "Floating Point Divide by Zero");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_FLT_INEXACT_RESULT: std::format_to_n(crashReportContent.begin(),
                                                            s_crashReportContentSize,
                                                            "Floating Point Inexact Result");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_FLT_INVALID_OPERATION: std::format_to_n(crashReportContent.begin(),
                                                               s_crashReportContentSize,
                                                               "Floating Point Invalid Operation");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_FLT_OVERFLOW: std::format_to_n(crashReportContent.begin(),
                                                      s_crashReportContentSize,
                                                      "Floating Point Overflow");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_FLT_STACK_CHECK: std::format_to_n(crashReportContent.begin(),
                                                         s_crashReportContentSize,
                                                         "The stack overflowed or underflowed as the result of a floating-point operation.");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_FLT_UNDERFLOW: std::format_to_n(crashReportContent.begin(),
                                                       s_crashReportContentSize,
                                                       "The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_ILLEGAL_INSTRUCTION: std::format_to_n(crashReportContent.begin(),
                                                             s_crashReportContentSize,
                                                             "Illegal Instruction");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_IN_PAGE_ERROR: std::format_to_n(crashReportContent.begin(),
                                                       s_crashReportContentSize,
                                                       "Thread tried accessing a page that was not present, and the system was unable to load the page.");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_INT_DIVIDE_BY_ZERO: std::format_to_n(crashReportContent.begin(),
                                                            s_crashReportContentSize,
                                                            "Integer Division by Zero");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_INT_OVERFLOW: std::format_to_n(crashReportContent.begin(),
                                                      s_crashReportContentSize,
                                                      "The result of an integer operation cause a carry out of the most significant bit of the result");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_INVALID_DISPOSITION: std::format_to_n(crashReportContent.begin(),
                                                             s_crashReportContentSize,
                                                             "EXCEPTION_INVALID_DISPOSITION (If you see this, good luck lmao)");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: std::format_to_n(crashReportContent.begin(),
                                                                  s_crashReportContentSize,
                                                                  "Thread tried to continue execution after a noncontinuable exception occurred.");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_PRIV_INSTRUCTION: std::format_to_n(crashReportContent.begin(),
                                                          s_crashReportContentSize,
                                                          "Thread tried to execute an instruction whose operation is not allowed in the current machine mode.");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_SINGLE_STEP: std::format_to_n(crashReportContent.begin(),
                                                     s_crashReportContentSize,
                                                     "A trace trap or other single-instruction mechanism signaled that one instruction has been executed. (If you see this, wtf?)");
            return EXCEPTION_EXECUTE_HANDLER;
        case EXCEPTION_STACK_OVERFLOW: std::format_to_n(crashReportContent.begin(),
                                                        s_crashReportContentSize,
                                                        "The thread used up its stack");
            return EXCEPTION_EXECUTE_HANDLER;
        default: std::format_to_n(crashReportContent.begin(),
                                  s_crashReportContentSize,
                                  "Unknown Exception: 0x{:x}",
                                  code);
            return EXCEPTION_EXECUTE_HANDLER;
    }
}

void printException(const cpptrace::stacktrace& trace)
{
    crashReportContent.shrink_to_fit();
    auto res = std::format_to_n(crashReportHeader.begin(),
                                s_crashReportHeaderMaxSize,
                                "{}{}",
                                crashReportContent.c_str(), //  NOLINT(*-redundant-string-cstr)
                                s_stacktraceHeader);
    crashReportHeader.resize(res.size);
    auto formatter = cpptrace::formatter{}
                     .header(std::move(crashReportHeader))
                     .hide_exception_machinery(true)
                     .symbols(cpptrace::formatter::symbol_mode::pretty)
                     .snippets(true);

    // TODO Print to file and to console
    formatter.print(trace);
    formatter.print(crashReportFile, trace);
    crashReportFile.close();
    FatalErrorDialog("A crash occurred!", "{}, Please send the crash_report.txt file to the developers.", crashReportContent);
}
}  // namespace _internalDoNotUse

bool SetThreadName(HANDLE thread, std::string_view name)
{
    if (thread == nullptr) { return false; }

    const std::wstring wname = utf8_to_wstring(name);
    if (wname.empty() && !name.empty()) { return false; }

    return SUCCEEDED(SetThreadDescription(thread, wname.c_str()));
}

bool SetThreadPriority(HANDLE thread, int priority)
{
    return ::SetThreadPriority(thread, priority) != 0;
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

ThreadHandle_t GetCurrentThread()
{
#if __has_include(<pthread.h>)
    return pthread_self();
#else
    return ::GetCurrentThread();
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
