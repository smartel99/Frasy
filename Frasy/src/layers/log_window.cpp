/**
 * @file    log_window.cpp
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
#include "log_window.h"

#include "frasy_interpreter.h"
#include "utils/internal_config.h"
#include "utils/log_window_sink.h"

#include <Brigerad/Core/Log.h>
#include <Brigerad/Debug/Instrumentor.h>


namespace Frasy
{
LogWindow::LogWindow() noexcept
{
    InternalConfig cfg = FrasyInterpreter::Get().GetConfig().GetField("LogWindow");
    m_options          = LogWindowOptions(cfg);

    m_renderLoggersFunc =
      m_options.CombineLoggers ? &RenderCombinedLoggers : &RenderSeparateLoggers;
}

void LogWindow::OnAttach()
{
    BR_PROFILE_FUNCTION();

    if (m_options.CombineLoggers)
    {
        m_sink = std::make_shared<LogWindowSingleSink>(m_options.EntriesToShow);
    }
    else { m_sink = std::make_shared<LogWindowMultiSink>(m_options.EntriesToShow); }
    m_sink->set_pattern(m_options.CombineLoggers ? s_combinedPattern : s_separatePattern);
    Brigerad::Log::AddSink(m_sink);
}

void LogWindow::OnDetach()
{
    BR_PROFILE_FUNCTION();

    FrasyInterpreter::Get().GetConfig().SetField("LogWindow", m_options.Serialize());

    Brigerad::Log::RemoveSink(m_sink);
}

void LogWindow::OnImGuiRender()
{
    BR_PROFILE_FUNCTION();

    if (!m_isVisible) { return; }

    if (ImGui::Begin(s_windowName, &m_isVisible))
    {
        RenderOptions();

        ImGui::Separator();

        ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoDocking;
        ImGui::BeginChild("ScrollingLog", ImVec2 {0.0f, 0.0f}, false, flags);
        m_renderLoggersFunc(m_options, m_sink);
        ImGui::EndChild();
    }
    ImGui::End();
}

void LogWindow::RenderOptions()
{
}

void LogWindow::RenderCombinedLoggers(LogWindowOptions&                     options,
                                      const std::shared_ptr<LogWindowSink>& sink)
{
    try
    {
        LogWindowSingleSink& singleSink = *dynamic_cast<LogWindowSingleSink*>(sink.get());
        RenderLoggerEntries(options, "loggers", singleSink.GetEntries());
    }
    catch (std::bad_cast& e)
    {
        BR_CORE_ERROR("A logging error occurred: {}", e.what());
    }
}

void LogWindow::RenderSeparateLoggers(LogWindowOptions&                     options,
                                      const std::shared_ptr<LogWindowSink>& sink)
{
    if (ImGui::BeginTabBar("loggerTabBar", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton))
    {
        try
        {
            LogWindowMultiSink& multiSink = *dynamic_cast<LogWindowMultiSink*>(sink.get());
            for (auto&& [logger, entries] : multiSink.GetLoggers())
            {
                // TODO: Indicate new entries from inactive tabs to the user through the unsaved
                //  flag.
                ImGuiTabItemFlags flags =
                  ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_NoReorder;
                if (ImGui::BeginTabItem(logger.data(), nullptr, flags))
                {
                    RenderLoggerEntries(options, logger, entries);
                    ImGui::EndTabItem();
                }
            }
        }
        catch (std::bad_cast& e)
        {
            BR_CORE_ERROR("A logging error occurred: {}", e.what());
        }
        ImGui::EndTabBar();
    }
}


void LogWindow::RenderLoggerEntries(LogWindowOptions&               options,
                                    const LogWindow::LoggerName&    loggerName,
                                    const LogWindow::LoggerEntries& loggerEntries)
{
    std::string            label      = fmt::format("{}-entries", loggerName);
    static ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
                                        ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter |
                                        ImGuiTableFlags_BordersV;

    if (ImGui::BeginTable(label.c_str(), 4, tableFlags))
    {
        auto flagFromOption = [](bool enabled)
        { return enabled ? ImGuiTableColumnFlags_None : ImGuiTableColumnFlags_DefaultHide; };
        ImGui::TableSetupColumn(
          "Timestamp", ImGuiTableColumnFlags_DefaultSort | flagFromOption(options.ShowTimeStamp));
        ImGui::TableSetupColumn("Source", flagFromOption(options.ShowLogSource));
        ImGui::TableSetupColumn("Entry", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("Location", flagFromOption(options.ShowSourceLocation));
        ImGui::TableHeadersRow();
        for (size_t i = 0; i < loggerEntries.size(); i++)
        {
            ImGui::TableNextRow();
            const LogEntry& entry = loggerEntries.at(i);
            if (options.ShowLevels[entry.Level])
            {
                if (!options.Filter.IsActive() || options.Filter.PassFilter(entry.Entry.c_str()))
                {
                    RenderEntry(options, entry);
                }
            }
        }
        ImGui::EndTable();
    }
}

void LogWindow::RenderEntry(LogWindowOptions& options, const LogEntry& entry)
{
    BR_PROFILE_FUNCTION();

    static auto renderColumn = [](size_t columnId, const std::string& t, bool& isEnabled)
    {
        if (ImGui::TableSetColumnIndex(static_cast<int>(columnId)))
        {
            float wrapPos = ImGui::GetContentRegionMax().x;
            ImGui::PushTextWrapPos(wrapPos);
            ImGui::TextWrapped("%s", t.c_str());
            ImGuiTableColumnFlags currentFlags = ImGui::TableGetColumnFlags();
            isEnabled = (currentFlags & ImGuiTableColumnFlags_IsEnabled) != 0;
            ImGui::PopTextWrapPos();
        }
    };

    ImGui::BeginGroup();
    // Use the pointer location as a unique ID.
    ImGui::PushID(static_cast<int>(reinterpret_cast<size_t>(entry.Entry.c_str())));

    bool isCritical = entry.Level == spdlog::level::critical;

    if (!isCritical) { ImGui::PushStyleColor(ImGuiCol_Text, s_colors[entry.Level]); }
    else
    {
        // Critical is special: use red background with white foreground.
        ImGui::PushStyleColor(ImGuiCol_Header, s_colors[spdlog::level::critical]);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, s_colors[spdlog::level::critical]);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, s_colors[spdlog::level::critical]);
    }

    renderColumn(0, entry.Timestamp, options.ShowTimeStamp);
    renderColumn(1, entry.LoggerName, options.ShowLogSource);
    bool dummy;
    renderColumn(2, entry.Entry, dummy);
    renderColumn(
      3, entry.FormatSourceLocation(options.SourceLocationRenderStyle), options.ShowSourceLocation);

    ImGui::PopStyleColor(isCritical ? 3 : 1);
    ImGui::PopID();
    ImGui::EndGroup();
}

void LogWindow::SetVisibility(bool visibility) noexcept
{
    m_isVisible = visibility;
    ImGui::SetWindowFocus(s_windowName);
}

}    // namespace Frasy
