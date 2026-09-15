/**
 * @file    cli_args.cpp
 * @author  Frasy
 * @date    2026-08-05
 * @brief   Command-line argument parser implementation.
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
#include "cli_args.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace Frasy {

namespace {
CliArgs s_instance;

void printUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  --headless              Run in headless (CLI) mode\n"
              << "  --mcp-server            Run as MCP tool server (stdio JSON-RPC, deprecated)\n"
              << "  --mcp-port <port>       Host MCP HTTP server on this port (0 = auto)\n"
              << "  --mcp-client            Run as stdio-to-HTTP relay connecting to a running instance\n"
              << "  --address <addr>        Target address for --mcp-client (default: 127.0.0.1)\n"
              << "  --port <port>           Target port for --mcp-client (required with --mcp-client)\n"
              << "  --product <name>        Product to test (required in headless mode)\n"
              << "  --operator <name>       Operator name (required in headless mode)\n"
              << "  --serial <sn>           Serial number for a UUT (repeatable, required in headless mode)\n"
              << "  --config <path>         Path to config file (default: config.json)\n"
              << "  --output-format <fmt>   Output format: human or json (default: human)\n"
              << "  --output-dir <path>     Output directory for reports (default: logs)\n"
              << "  --skip-verification     Skip hash verification stage\n"
              << "  --verbose               Show logs on stderr (headless/MCP mode only)\n"
              << "  --popup-timeout <secs>  Auto-cancel popups after N seconds (default: 0 = no timeout)\n"
              << "  --help                  Show this help message and exit\n"
              << "\n"
              << "Examples:\n"
              << "  " << programName << " --headless --product MyProduct --operator CI --serial SN001\n"
              << "  " << programName << " --mcp-port 8080\n"
              << "  " << programName << " --mcp-client --port 8080\n"
              << "  " << programName << " --mcp-client --address 192.168.0.105 --port 69\n"
              << "\n"
              << "Exit codes (headless mode):\n"
              << "  0  All UUTs passed\n"
              << "  1  One or more UUTs failed\n"
              << "  2  Error (setup failure, invalid arguments, etc.)\n";
}

/// Peek at the next argument value. Returns nullptr if no value follows.
const char* peekNextArg(int i, int argc, char** argv, const char* flagName)
{
    if (i + 1 >= argc) {
        std::cerr << "Error: " << flagName << " requires a value\n";
        return nullptr;
    }
    return argv[i + 1];
}
}    // namespace

CliArgs CliArgs::parse(int argc, char** argv)
{
    CliArgs args;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "--help") {
            printUsage(argv[0]);
            std::exit(0);
        }
        else if (arg == "--headless") {
            args.headless = true;
        }
        else if (arg == "--mcp-server") {
            args.mcpServer = true;
        }
        else if (arg == "--mcp-client") {
            args.mcpClient = true;
        }
        else if (arg == "--mcp-port") {
            const char* val = peekNextArg(i, argc, argv, "--mcp-port");
            if (!val) { std::exit(2); }
            try {
                args.mcpPort = std::stoi(val);
                if (args.mcpPort < 0) {
                    std::cerr << "Error: --mcp-port must be a non-negative integer\n";
                    std::exit(2);
                }
            }
            catch (...) {
                std::cerr << "Error: --mcp-port must be a valid integer, got '" << val << "'\n";
                std::exit(2);
            }
            ++i;
        }
        else if (arg == "--address") {
            const char* val = peekNextArg(i, argc, argv, "--address");
            if (!val) { std::exit(2); }
            args.address = val;
            ++i;
        }
        else if (arg == "--port") {
            const char* val = peekNextArg(i, argc, argv, "--port");
            if (!val) { std::exit(2); }
            try {
                args.port = std::stoi(val);
                if (args.port <= 0) {
                    std::cerr << "Error: --port must be a positive integer\n";
                    std::exit(2);
                }
            }
            catch (...) {
                std::cerr << "Error: --port must be a valid integer, got '" << val << "'\n";
                std::exit(2);
            }
            ++i;
        }
        else if (arg == "--skip-verification") {
            args.skipVerification = true;
        }
        else if (arg == "--verbose") {
            args.verbose = true;
        }
        else if (arg == "--product") {
            const char* val = peekNextArg(i, argc, argv, "--product");
            if (!val) { std::exit(2); }
            args.product = val;
            ++i;
        }
        else if (arg == "--operator") {
            const char* val = peekNextArg(i, argc, argv, "--operator");
            if (!val) { std::exit(2); }
            args.operatorName = val;
            ++i;
        }
        else if (arg == "--serial") {
            const char* val = peekNextArg(i, argc, argv, "--serial");
            if (!val) { std::exit(2); }
            args.serials.emplace_back(val);
            ++i;
        }
        else if (arg == "--config") {
            const char* val = peekNextArg(i, argc, argv, "--config");
            if (!val) { std::exit(2); }
            args.configPath = val;
            ++i;
        }
        else if (arg == "--output-format") {
            const char* val = peekNextArg(i, argc, argv, "--output-format");
            if (!val) { std::exit(2); }
            args.outputFormat = val;
            if (args.outputFormat != "human" && args.outputFormat != "json") {
                std::cerr << "Error: --output-format must be 'human' or 'json', got '" << args.outputFormat << "'\n";
                std::exit(2);
            }
            ++i;
        }
        else if (arg == "--output-dir") {
            const char* val = peekNextArg(i, argc, argv, "--output-dir");
            if (!val) { std::exit(2); }
            args.outputDir = val;
            ++i;
        }
        else if (arg == "--popup-timeout") {
            const char* val = peekNextArg(i, argc, argv, "--popup-timeout");
            if (!val) { std::exit(2); }
            try {
                args.popupTimeoutSeconds = std::stoi(val);
                if (args.popupTimeoutSeconds < 0) {
                    std::cerr << "Error: --popup-timeout must be a non-negative integer\n";
                    std::exit(2);
                }
            }
            catch (...) {
                std::cerr << "Error: --popup-timeout must be a valid integer, got '" << val << "'\n";
                std::exit(2);
            }
            ++i;
        }
        // Ignore unknown flags silently (they may be for the application or Brigerad)
    }

    // Validate mutual exclusivity
    if (args.headless && args.mcpServer) {
        std::cerr << "Error: --headless and --mcp-server are mutually exclusive\n";
        std::exit(2);
    }
    if (args.headless && args.mcpClient) {
        std::cerr << "Error: --headless and --mcp-client are mutually exclusive\n";
        std::exit(2);
    }
    if (args.headless && args.mcpPort >= 0) {
        std::cerr << "Error: --headless and --mcp-port are mutually exclusive\n";
        std::exit(2);
    }
    if (args.mcpServer && args.mcpClient) {
        std::cerr << "Error: --mcp-server and --mcp-client are mutually exclusive\n";
        std::exit(2);
    }
    if (args.mcpServer && args.mcpPort >= 0) {
        std::cerr << "Error: --mcp-server and --mcp-port are mutually exclusive\n";
        std::exit(2);
    }
    if (args.mcpClient && args.mcpPort >= 0) {
        std::cerr << "Error: --mcp-client and --mcp-port are mutually exclusive\n";
        std::exit(2);
    }

    // Validate --mcp-client requires --port
    if (args.mcpClient && args.port <= 0) {
        std::cerr << "Error: --mcp-client requires --port <port>\n";
        std::exit(2);
    }

    // Validate required flags in headless mode
    if (args.headless) {
        bool hasError = false;
        if (args.product.empty()) {
            std::cerr << "Error: --product is required in headless mode\n";
            hasError = true;
        }
        if (args.operatorName.empty()) {
            std::cerr << "Error: --operator is required in headless mode\n";
            hasError = true;
        }
        if (args.serials.empty()) {
            std::cerr << "Error: at least one --serial is required in headless mode\n";
            hasError = true;
        }
        if (hasError) {
            std::cerr << "\nRun with --help for usage information.\n";
            std::exit(2);
        }
    }

    s_instance = args;
    return args;
}

const CliArgs& CliArgs::get()
{
    return s_instance;
}

}    // namespace Frasy
