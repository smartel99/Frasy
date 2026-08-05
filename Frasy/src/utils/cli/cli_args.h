/**
 * @file    cli_args.h
 * @author  Frasy
 * @date    2026-08-05
 * @brief   Command-line argument parser for Frasy headless mode.
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
#ifndef FRASY_UTILS_CLI_ARGS_H
#define FRASY_UTILS_CLI_ARGS_H

#include <string>
#include <vector>

namespace Frasy {

struct CliArgs {
    bool                     headless            = false;
    bool                     mcpServer           = false;
    std::string              product;
    std::string              operatorName;
    std::vector<std::string> serials;
    std::string              configPath          = "config.json";
    std::string              outputFormat         = "human";    // "human" or "json"
    std::string              outputDir            = "logs";
    bool                     skipVerification    = false;
    int                      popupTimeoutSeconds = 0;    // 0 = no timeout

    /// Parse command-line arguments. Stores the result globally accessible via get().
    /// If --help is present, prints usage and calls std::exit(0).
    /// If headless mode has missing required flags, prints error and calls std::exit(2).
    static CliArgs parse(int argc, char** argv);

    /// Retrieve the globally stored parsed CLI args.
    static const CliArgs& get();
};

}    // namespace Frasy

#endif    // FRASY_UTILS_CLI_ARGS_H
