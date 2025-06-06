/**
 * @file    log_window_options.h
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

#ifndef FRASY_UTILS_LOG_WINDOW_OPTIONS_H
#define FRASY_UTILS_LOG_WINDOW_OPTIONS_H

#include "imgui.h"
#include "log_entry.h"
#include "spdlog/common.h"
#include <json.hpp>

#include <array>

namespace Frasy {
struct LogWindowOptions {
    static constexpr size_t DEFAULT_ENTRIES_TO_SHOW = 4096;
    size_t                  EntriesToShow           = DEFAULT_ENTRIES_TO_SHOW;

    ImGuiTextFilter Filter     = {};
    bool            AutoScroll = true;

    bool CombineLoggers = true;

    std::array<bool, spdlog::level::n_levels> ShowLevels = {true, true, true, true, true, true, true};

    bool                                 ShowTimeStamp             = true;
    bool                                 ShowLogSource             = true;
    bool                                 ShowSourceLocation        = true;
    LogEntry::SourceLocationRenderStyles SourceLocationRenderStyle = LogEntry::SourceLocationRenderStyle_All;

    static LogWindowOptions from_json(const nlohmann::json& cfg) noexcept
    {
        LogWindowOptions options;
#define LOAD_FIELD(v) options.v = cfg.value<decltype(v)>(#v, options.v)
        LOAD_FIELD(EntriesToShow);
        LOAD_FIELD(AutoScroll);
        LOAD_FIELD(CombineLoggers);
        LOAD_FIELD(ShowLevels);
        LOAD_FIELD(ShowTimeStamp);
        LOAD_FIELD(ShowLogSource);
        LOAD_FIELD(ShowSourceLocation);
        LOAD_FIELD(SourceLocationRenderStyle);
#undef LOAD_FIELD

        // Load the configured logger levels.
        auto loggers = cfg.value("Loggers", nlohmann::json::object());
        for (auto& [name, logger] : loggers.items()) {
            try {
                const auto level = static_cast<spdlog::level::level_enum>(logger.get<int>());
                Brigerad::Log::SetLoggerLevel(name, level);
            }
            catch (...) {
                DebugBreak();
            }
        }
        return options;
    }

    [[nodiscard]] nlohmann::json to_json() const noexcept
    {
        nlohmann::json cfg = nlohmann::json::object();

#define SET_FIELD(v) cfg[#v] = v
        SET_FIELD(EntriesToShow);
        SET_FIELD(AutoScroll);
        SET_FIELD(CombineLoggers);
        SET_FIELD(ShowLevels);
        SET_FIELD(ShowTimeStamp);
        SET_FIELD(ShowLogSource);
        SET_FIELD(ShowSourceLocation);
        SET_FIELD(SourceLocationRenderStyle);
#undef SET_FIELD

        // Save the overriden log levels, if any.
        nlohmann::json loggerLevels = nlohmann::json::object();
        const auto&    loggers      = Brigerad::Log::GetLoggers();
        for (auto&& [name, ptr] : loggers) {
            if (ptr->level() != Brigerad::Log::s_defaultLevel) { loggerLevels[name] = ptr->level(); }
        }
        cfg["Loggers"] = loggerLevels;

        return cfg;
    }
};
}    // namespace Frasy
#endif    // FRASY_UTILS_LOG_WINDOW_OPTIONS_H
