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
#include <utils/headless/headless_runner.h>
#include <utils/mcp/mcp_relay.h>
#include <utils/mcp/mcp_runner.h>
#include <frasy_interpreter.h>

#if defined(BR_PLATFORM_WINDOWS) || defined(BR_PLATFORM_LINUX)

extern Brigerad::Application* Brigerad::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
    Frasy::CliArgs::parse(argc, argv);
    const auto& cliArgs = Frasy::CliArgs::get();
    int exitCode = 0;

    // --mcp-client: lightweight relay process, no app/GUI needed
    if (cliArgs.mcpClient) {
        BR_BEGIN_GUARDED_SCOPE
            {
                Frasy::Mcp::McpRelay relay(cliArgs.address, cliArgs.port);
                exitCode = relay.run();
            }
        BR_END_GUARDED_SCOPE
        return exitCode;
    }

    BR_BEGIN_GUARDED_SCOPE
        {
            BR_PROFILE_BEGIN_SESSION("Init", "BrigeradProfile-Startup.json");
            cpptrace::register_terminate_handler();
            cpptrace::absorb_trace_exceptions(true);
            Brigerad::_internalDoNotUse::initExceptionHandling();
            Brigerad::Log::Init(cliArgs.headless || cliArgs.mcpServer,
                               (cliArgs.headless || cliArgs.mcpServer) && !cliArgs.verbose);

            BR_PROFILE_END_SESSION();
            auto app = Brigerad::CreateApplication(argc, argv);

            if (cliArgs.mcpServer) {
                auto* provider = Frasy::Interpreter::Get().getProductProvider();
                if (!provider) {
                    BR_CORE_ERROR("No ProductProvider registered for MCP server mode");
                    exitCode = 2;
                }
                else {
                    Frasy::Mcp::McpRunner mcpRunner(*provider);
                    exitCode = mcpRunner.run();
                }
            }
            else if (cliArgs.headless) {
                auto* provider = Frasy::Interpreter::Get().getProductProvider();
                if (!provider) {
                    BR_CORE_ERROR("No ProductProvider registered for headless mode");
                    exitCode = 2;
                }
                else {
                    Frasy::Headless::HeadlessRunner runner(cliArgs, *provider);
                    exitCode = runner.run();
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
