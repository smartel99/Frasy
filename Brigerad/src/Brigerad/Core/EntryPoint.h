#pragma once

#include "Log.h"
#include "cpptrace/formatting.hpp"
#include "cpptrace/utils.hpp"

#include <cpptrace/from_current.hpp>

#include <filesystem>
#include <string>
#include <print>
#include <fstream>
#include <format>
#include <cstddef>

#if defined(BR_PLATFORM_WINDOWS) || defined(BR_PLATFORM_LINUX)

extern Brigerad::Application* Brigerad::CreateApplication(int argc, char** argv);

namespace Brigerad::Internals_do_not_use::Details {
static constexpr std::size_t      s_crashReportHeaderMaxSize = 1024;
static constexpr std::string_view s_stacktraceHeader         = "\nStack trace (most recent call first):\n";
static constexpr std::size_t      s_crashReportContentSize   = s_crashReportHeaderMaxSize - s_stacktraceHeader.size();
inline std::string                crashReportContent;
inline std::string                crashReportHeader;
inline std::ofstream              crashReportFile;

inline int test(int x, int y)
{
    return x / y;
}

inline void test2()
{
    int* p = 0;
    *p     = 5;
}

inline void printException(const cpptrace::stacktrace& trace)
{
    crashReportContent.shrink_to_fit();
    auto res = std::format_to_n(crashReportHeader.begin(),
                                s_crashReportHeaderMaxSize,
                                "{}{}",
                                crashReportContent.c_str(),
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
}

inline int parse_filter(unsigned long code)
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
}

int main(int argc, char** argv)
{
    using namespace Brigerad::Internals_do_not_use::Details;
    CPPTRACE_SEH_TRY {
        [&] {
            CPPTRACE_TRY
                {
                    BR_PROFILE_BEGIN_SESSION("Init", "BrigeradProfile-Startup.json");
                    cpptrace::register_terminate_handler();
                    cpptrace::absorb_trace_exceptions(true);
                    Brigerad::Log::Init();
                    crashReportFile = std::ofstream{"crash_report.txt"};
                    BR_CORE_ASSERT(crashReportFile.is_open(), "Unable to open crash report file!");
                    // Reserve memory for the string so that we are guaranteed to have the space for it, even in the case we fucked the dog way too hard
                    crashReportHeader.resize(s_crashReportHeaderMaxSize, 0);
                    crashReportContent.resize(s_crashReportContentSize, 0);

                    std::string path = std::filesystem::current_path().string();
                    //    BR_CORE_WARN("Running from: {0}", path);
                    // test(10, 0);
                    // test2();

                    BR_PROFILE_END_SESSION();
                    auto app = Brigerad::CreateApplication(argc, argv);
                    app->run();

                    BR_PROFILE_BEGIN_SESSION("Shutdown", "BrigeradProfile-Shutdown.json");
                    delete app;
                    BR_PROFILE_END_SESSION();
                }
            CPPTRACE_CATCH(std::exception& e) {
                std::format_to_n(crashReportContent.begin(),
                                 s_crashReportContentSize,
                                 "Unhandled exception: {}",
                                 e.what());
                printException(cpptrace::from_current_exception());
            }
        }();
    }
    CPPTRACE_SEH_EXCEPT(parse_filter(GetExceptionCode())) {
        printException(cpptrace::from_current_exception());
    }
}

#endif
