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

#include "../internal_config.h"
#include "log_entry.h"

#include "imgui.h"
#include "spdlog/common.h"

#include <array>

namespace Frasy
{
struct LogWindowOptions
{
    static constexpr size_t DEFAULT_ENTRIES_TO_SHOW = 4096;
    size_t                  EntriesToShow           = DEFAULT_ENTRIES_TO_SHOW;

    ImGuiTextFilter Filter     = {};
    bool            AutoScroll = true;

    bool CombineLoggers = false;

    std::array<bool, spdlog::level::n_levels> ShowLevels = {
      true, true, true, true, true, true, true};

    bool                                 ShowTimeStamp      = true;
    bool                                 ShowLogSource      = true;
    bool                                 ShowSourceLocation = true;
    LogEntry::SourceLocationRenderStyles SourceLocationRenderStyle =
      LogEntry::SourceLocationRenderStyle_All;


    LogWindowOptions() noexcept = default;
    explicit LogWindowOptions(const InternalConfig& cfg) noexcept
    {
#define LOAD_FIELD(v) v = cfg.GetField<decltype(v)>(#v, this->v)
        LOAD_FIELD(EntriesToShow);
        LOAD_FIELD(AutoScroll);
        LOAD_FIELD(CombineLoggers);
        LOAD_FIELD(ShowLevels);
        LOAD_FIELD(ShowTimeStamp);
        LOAD_FIELD(ShowLogSource);
        LOAD_FIELD(ShowSourceLocation);
        LOAD_FIELD(SourceLocationRenderStyle);
#undef LOAD_FIELD
    }

    [[nodiscard]] InternalConfig Serialize() const noexcept
    {
        InternalConfig cfg = {};

#define SET_FIELD(v) cfg.SetField(#v, v)
        SET_FIELD(EntriesToShow);
        SET_FIELD(AutoScroll);
        SET_FIELD(CombineLoggers);
        SET_FIELD(ShowLevels);
        SET_FIELD(ShowTimeStamp);
        SET_FIELD(ShowLogSource);
        SET_FIELD(ShowSourceLocation);
        SET_FIELD(SourceLocationRenderStyle);
#undef SET_FIELD
        return cfg;
    }
};
}    // namespace Frasy
#endif    // FRASY_UTILS_LOG_WINDOW_OPTIONS_H
