/**
 * @file    console_popup_handler.h
 * @author  Frasy
 * @date    2026-08-05
 * @brief   Console-based popup handler for headless mode.
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
#ifndef FRASY_UTILS_HEADLESS_CONSOLE_POPUP_HANDLER_H
#define FRASY_UTILS_HEADLESS_CONSOLE_POPUP_HANDLER_H

#include <map>
#include <mutex>
#include <sol/sol.hpp>
#include <string>

namespace Frasy::Headless {

/**
 * @brief Parsed response from stdin for popup interaction.
 */
struct ParsedResponse {
    std::string                button;
    std::map<int, std::string> inputs;    // 1-indexed
};

/// Parse a human-format stdin line (e.g., "1=value" or "OK")
ParsedResponse parseHumanLine(const std::string& line);

/// Parse a JSON-format stdin line (e.g., {"button":"OK","inputs":{"1":"value"}})
ParsedResponse parseJsonLine(const std::string& line);

/**
 * @brief Installs a console-based popup handler into the Lua state.
 *
 * This replaces the GUI-based popup rendering with text I/O on stdin/stdout.
 * In human mode: prints a formatted popup box and reads commands interactively.
 * In JSON mode: emits a JSON object and reads a JSON response line.
 *
 * @param lua The Lua state to install the popup handler into.
 * @param uut The UUT index this state belongs to.
 * @param outputFormat "human" or "json"
 * @param timeoutSeconds Popup timeout in seconds (0 = no timeout).
 * @param ioMutex Mutex to serialize stdin/stdout access across UUT threads.
 */
void importHeadlessPopup(sol::state_view    lua,
                         std::size_t        uut,
                         const std::string& outputFormat,
                         int                timeoutSeconds,
                         std::mutex&        ioMutex);

}    // namespace Frasy::Headless

#endif    // FRASY_UTILS_HEADLESS_CONSOLE_POPUP_HANDLER_H
