/**
 * @file    log_window.h
 * @author  Samuel Martel
 * @date    2022-12-05
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

#ifndef FRASY_LAYERS_LOG_WINDOW_H
#define FRASY_LAYERS_LOG_WINDOW_H

#include "Brigerad.h"
#include "Brigerad/Renderer/Texture.h"

#include "utils/log_window_options.h"
#include "utils/log_window_sink.h"

#include <imgui/imgui.h>
#include <spdlog/common.h>

#include <array>
#include <functional>
#include <memory>

namespace Frasy
{

class LogWindow : public Brigerad::Layer
{
public:
    LogWindow() noexcept;
    ~LogWindow() override = default;

    [[nodiscard]] const LogWindowOptions& GetLogOptions() const noexcept { return m_options; }

    void OnAttach() override;
    void OnDetach() override;
    void OnImGuiRender() override;

    void SetVisibility(bool visibility) noexcept;

protected:
    void RenderOptions();

    static void RenderCombinedLoggers(LogWindowOptions&                     options,
                                      const std::shared_ptr<LogWindowSink>& sink);
    static void RenderSeparateLoggers(LogWindowOptions&                     options,
                                      const std::shared_ptr<LogWindowSink>& sink);
    void (*m_renderLoggersFunc)(LogWindowOptions&, const std::shared_ptr<LogWindowSink>&) = nullptr;

    using LoggerName    = LogWindowMultiSink::LoggerMap::key_type;
    using LoggerEntries = LogWindowMultiSink::LoggerMap::mapped_type;
    static void RenderLoggerEntries(LogWindowOptions&    options,
                                    const LoggerName&    loggerName,
                                    const LoggerEntries& loggerEntries);

    static void RenderEntry(LogWindowOptions& options, const LogEntry& entry);

protected:
    bool                           m_isVisible = false;
    LogWindowOptions               m_options;
    std::shared_ptr<LogWindowSink> m_sink;

    static constexpr std::array s_colors = {
      ImVec4(1.0f, 1.0f, 1.0f, 1.0f),                                   //!< Trace
      ImVec4(1.0f, 1.0f, 1.0f, 1.0f),                                   //!< Debug
      ImVec4(0.0f / 255.0f, 161.0f / 255.0f, 8.0f / 255.0f, 1.0),       //!< Info
      ImVec4(236.0f / 255.0f, 252.0f / 255.0f, 92.0f / 255.0f, 1.0),    //!< Warning
      ImVec4(220.0f / 255.0f, 48.0f / 255.0f, 48.0f / 255.0f, 1.0),     //!< Error
      ImVec4(255.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 1.0),       //!< Critical
    };
    static_assert(s_colors.size() == spdlog::level::n_levels - 1);

private:
    static constexpr const char* s_combinedPattern = "%n: %v";
    static constexpr const char* s_separatePattern = "%v";

    static constexpr const char* s_windowName = "Logs";
};
}    // namespace Frasy
#endif    // FRASY_LAYERS_LOG_WINDOW_H
