#pragma once

#include "Log.h"
#include "Thread.h"
#include "cpptrace/formatting.hpp"
#include "cpptrace/utils.hpp"

#include <cpptrace/from_current.hpp>

#include <filesystem>
#include <string>
#include <print>
#include <fstream>
#include <format>
#include <cstddef>

#include <utils/cli/cli_args.h>
#include <frasy_interpreter.h>

#if defined(BR_PLATFORM_WINDOWS) || defined(BR_PLATFORM_LINUX)

extern Brigerad::Application* Brigerad::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
    Frasy::CliArgs::parse(argc, argv);
    const auto& cliArgs = Frasy::CliArgs::get();
    int exitCode = 0;

    BR_BEGIN_GUARDED_SCOPE
        {
            BR_PROFILE_BEGIN_SESSION("Init", "BrigeradProfile-Startup.json");
            cpptrace::register_terminate_handler();
            cpptrace::absorb_trace_exceptions(true);
            Brigerad::Log::Init();
            Brigerad::_internalDoNotUse::initExceptionHandling();

            BR_PROFILE_END_SESSION();
            auto app = Brigerad::CreateApplication(argc, argv);

            if (cliArgs.headless) {
                auto* provider = Frasy::Interpreter::Get().getProductProvider();
                if (!provider) {
                    BR_CORE_ERROR("No ProductProvider registered for headless mode");
                    exitCode = 2;
                }
                else {
                    // TODO: HeadlessRunner will be wired here in Task 4
                    BR_CORE_INFO("Headless mode: product='{}' operator='{}' serials={}",
                                 cliArgs.product, cliArgs.operatorName, cliArgs.serials.size());
                    exitCode = 0;
                }
            }
            else {
                app->run();
            }

            BR_PROFILE_BEGIN_SESSION("Shutdown", "BrigeradProfile-Shutdown.json");
            delete app;
            BR_PROFILE_END_SESSION();
        }
    BR_END_GUARDED_SCOPE

    return exitCode;
}

#endif
