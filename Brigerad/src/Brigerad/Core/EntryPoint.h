#pragma once

#include "Log.h"
#include "cpptrace/formatting.hpp"
#include "cpptrace/utils.hpp"

#include <cpptrace/from_current.hpp>

#include <filesystem>
#include <string>
#include <print>
#include <fstream>

#include <signal.h>

#if defined(BR_PLATFORM_WINDOWS) || defined(BR_PLATFORM_LINUX)

extern Brigerad::Application* Brigerad::CreateApplication(int argc, char** argv);

namespace Brigerad::Internals_do_not_use::Details {
inline std::string   crashReportHeader;
inline std::ofstream crashReportFile;

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
    auto formatter = cpptrace::formatter{}
                     // TODO Add header to show crashReportHeader + "Stack trace (most recent call first):"
                     .hide_exception_machinery(true)
                     .symbols(cpptrace::formatter::symbol_mode::pretty)
                     .snippets(true);

    // TODO Print to file and to console

}

inline int divide_zero_filter(int code)
{
    if (code == STATUS_INTEGER_DIVIDE_BY_ZERO || code == EXCEPTION_FLT_DIVIDE_BY_ZERO) {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    return EXCEPTION_CONTINUE_SEARCH;
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

                    std::string path = std::filesystem::current_path().string();
                    //    BR_CORE_WARN("Running from: {0}", path);
                    test(10, 0);
                    // test2();

                    BR_PROFILE_END_SESSION();
                    auto app = Brigerad::CreateApplication(argc, argv);
                    app->run();

                    BR_PROFILE_BEGIN_SESSION("Shutdown", "BrigeradProfile-Shutdown.json");
                    delete app;
                    BR_PROFILE_END_SESSION();
                }
            CPPTRACE_CATCH(std::exception& e) {
                // TODO Save the exception's what in the header string
                std::cerr << "Unhandled exception occurred: " << e.what() << std::endl;
                printException(cpptrace::from_current_exception());
            }
        }();
    }
    CPPTRACE_SEH_EXCEPT(divide_zero_filter(GetExceptionCode())) {
        // TODO filter should catch all SEH things, and save the code in the header.
        std::cerr << "Division by zero happened!" << std::endl;
        Brigerad::Internals_do_not_use::Details::printException(cpptrace::from_current_exception());
    }
}

#endif
