/**
 * @file    log_entry.h
 * @author  Samuel Martel
 * @date    2022-12-06
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
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#ifndef FRASY_UTILS_LOG_ENTRY_H
#define FRASY_UTILS_LOG_ENTRY_H

#include "spdlog/common.h"

#include <array>
#include <format>
#include <string>

namespace Frasy {
struct LogEntry {
    enum SourceLocationRenderStyles {
        SourceLocationRenderStyle_Function        = 0,
        SourceLocationRenderStyle_FunctionAndLine = 1,
        SourceLocationRenderStyle_File            = 2,
        SourceLocationRenderStyle_FileAndLine     = 3,
        SourceLocationRenderStyle_All             = 4,
        SourceLocationRenderStyle_Count
    };

    spdlog::level::level_enum Level = {};
    std::string               LoggerName;
    std::string               Filename;
    std::string               Funcname;
    int                       Line;
    std::string               Entry;
    std::string               Timestamp;

    [[nodiscard]] std::string FormatSourceLocation(SourceLocationRenderStyles style) const
    {
        try {
            switch (style) {
                case SourceLocationRenderStyle_Function: return std::format("{}", Funcname);
                case SourceLocationRenderStyle_FunctionAndLine: return std::format("{}:{}", Funcname, Line);
                case SourceLocationRenderStyle_File: return std::format("{}", Filename);
                case SourceLocationRenderStyle_FileAndLine: return std::format("{}:{}", Filename, Line);
                case SourceLocationRenderStyle_All:
                case SourceLocationRenderStyle_Count:
                default: return std::format("{}:{} ({})", Funcname, Line, Filename);
            }

        }
        catch (std::out_of_range& e) {
            return std::format("Invalid Style ({}) requested: {}", std::to_underlying(style), e.what());
        }
        catch (std::format_error& e) {
            return std::format("A format error occurred: {}", e.what());
        }
    }

    static constexpr std::array SourceLocationRenderStyleLabels = {
      "{function}", "{function}:{line}", "{file}", "{file}:{line}", "{function}:{line} ({file})"};
};
}    // namespace Frasy
#endif    // FRASY_UTILS_LOG_ENTRY_H
