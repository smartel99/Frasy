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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "log_window.h"

#include "frasy_interpreter.h"
#include "utils/logging/log_window_sink.h"

#include <Brigerad/Core/Log.h>
#include <Brigerad/Debug/Instrumentor.h>


namespace Frasy {
LogWindow::LogWindow() noexcept
{
    const auto cfg = Interpreter::Get().getConfig().value("LogWindow", nlohmann::json::object());
    m_options      = LogWindowOptions::from_json(cfg);

    m_renderLoggersFunc = m_options.CombineLoggers ? &RenderCombinedLoggers : &RenderSeparateLoggers;
}

void LogWindow::onAttach()
{
    BR_PROFILE_FUNCTION();

    if (m_options.CombineLoggers) { m_sink = std::make_shared<LogWindowSingleSink>(m_options.EntriesToShow); }
    else {
        m_sink = std::make_shared<LogWindowMultiSink>(m_options.EntriesToShow);
    }
    m_sink->set_pattern(m_options.CombineLoggers ? s_combinedPattern : s_separatePattern);
    Brigerad::Log::AddSink(m_sink);
}

void LogWindow::onDetach()
{
    BR_PROFILE_FUNCTION();

    Interpreter::Get().getConfig()["LogWindow"] = m_options.to_json();

    Brigerad::Log::RemoveSink(m_sink);
}

void LogWindow::onImGuiRender()
{
    BR_PROFILE_FUNCTION();

    if (!m_isVisible) { return; }

    if (ImGui::Begin(s_windowName, &m_isVisible, ImGuiWindowFlags_NoDocking)) {
        RenderOptions();

        ImGui::Separator();

        ImGuiWindowFlags flags = ImGuiWindowFlags_HorizontalScrollbar;
        if (ImGui::BeginChild("ScrollingLog", ImVec2 {0.0f, 0.0f}, false, flags)) {
            m_renderLoggersFunc(m_options, m_sink);
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void LogWindow::RenderOptions()
{
    if (ImGui::TreeNode("Options")) {
        const auto& loggers = Brigerad::Log::GetLoggers();

        for (auto&& [name, ptr] : loggers) {
            if (name.empty()) { continue; }    // No clue what this logger is, and it affects nothing
            auto currentLevel    = ptr->level();
            auto currentLevelStr = to_string_view(currentLevel);
            if (ImGui::BeginCombo(name.c_str(), currentLevelStr.data())) {
                for (spdlog::level::level_enum level = spdlog::level::trace; level != spdlog::level::n_levels;
                     level = static_cast<spdlog::level::level_enum>(static_cast<int>(level) + 1)) {
                    if (ImGui::Selectable(to_string_view(level).data())) { Brigerad::Log::SetLoggerLevel(name, level); }
                }
                ImGui::EndCombo();

                Interpreter::Get().getConfig()["LogWindow"] = m_options.to_json();
                Interpreter::Get().saveConfig();
            }
        }

        ImGui::TreePop();
    }
}

void LogWindow::RenderCombinedLoggers(LogWindowOptions& options, const std::shared_ptr<LogWindowSink>& sink)
{
    try {
        LogWindowSingleSink& singleSink = *dynamic_cast<LogWindowSingleSink*>(sink.get());
        RenderLoggerEntries(options, "loggers", singleSink.GetEntries());
    }
    catch (std::bad_cast& e) {
        BR_CORE_ERROR("A logging error occurred: {}", e.what());
    }
}

void LogWindow::RenderSeparateLoggers(LogWindowOptions& options, const std::shared_ptr<LogWindowSink>& sink)
{
    if (ImGui::BeginTabBar("loggerTabBar", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)) {
        try {
            LogWindowMultiSink& multiSink = *dynamic_cast<LogWindowMultiSink*>(sink.get());

            static LoggerName defaultLogger = {};
            const LoggerName* activeLogger  = &defaultLogger;

            static LoggerEntries defaultEntries = {};
            const LoggerEntries* activeEntries  = &defaultEntries;

            for (auto&& [logger, entries] : multiSink.GetLoggers()) {
                // TODO: Indicate new entries from inactive tabs to the user through the unsaved
                //  flag.
                ImGuiTabItemFlags flags = ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_NoReorder;
                if (ImGui::BeginTabItem(logger.data(), nullptr, flags)) {
                    // Tab is active, render it. We do not directly render the entries here to
                    // be able to sync table properties between tabs.
                    activeLogger  = &logger;
                    activeEntries = &entries;
                    ImGui::EndTabItem();
                }
            }
            RenderLoggerEntries(options, *activeLogger, *activeEntries);
        }
        catch (std::bad_cast& e) {
            BR_CORE_ERROR("A logging error occurred: {}", e.what());
        }
        ImGui::EndTabBar();
    }
}


void LogWindow::RenderLoggerEntries(LogWindowOptions&    options,
                                    const LoggerName&    loggerName,
                                    const LoggerEntries& loggerEntries)
{
    static ImGuiTableFlags tableFlags =
      ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Borders |
      ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SortMulti | ImGuiTableFlags_SortTristate;

    float maxY = ImGui::GetContentRegionAvail().y;
    if (ImGui::BeginTable("entries", 5, tableFlags, ImVec2 {0.0f, maxY})) {
        auto flagFromOption = [](bool enabled) {
            return enabled ? ImGuiTableColumnFlags_None : ImGuiTableColumnFlags_DefaultHide;
        };
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_DefaultSort | flagFromOption(options.ShowTimeStamp));
        ImGui::TableSetupColumn("Source", flagFromOption(options.ShowLogSource));
        ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("Location", flagFromOption(options.ShowSourceLocation));
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        for (size_t i = loggerEntries.size(); i > 0; i--) {
            if ((i - 1) >= loggerEntries.size()) { continue; }
            const LogEntry& entry = loggerEntries.at(i - 1);
            if (options.ShowLevels[entry.Level] && entry.Level >= Brigerad::Log::GetLoggerLevel(entry.LoggerName)) {
                ImGui::TableNextRow();
                if (!options.Filter.IsActive() || options.Filter.PassFilter(entry.Entry.c_str())) {
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

    static auto renderColumn = [](size_t columnId, const std::string& t, bool& isEnabled) {
        if (ImGui::TableSetColumnIndex(static_cast<int>(columnId))) {
            float wrapPos = ImGui::GetContentRegionMax().x;
            ImGui::PushTextWrapPos(wrapPos);
            ImGui::TextWrapped("%s", t.c_str());
            ImGuiTableColumnFlags currentFlags = ImGui::TableGetColumnFlags();
            isEnabled                          = (currentFlags & ImGuiTableColumnFlags_IsEnabled) != 0;
            ImGui::PopTextWrapPos();
        }
    };

    ImGui::BeginGroup();
    // Use the pointer location as a unique ID.
    ImGui::PushID(static_cast<int>(reinterpret_cast<size_t>(entry.Entry.c_str())));

    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, s_colors[entry.Level]);
    bool isCritical = entry.Level == spdlog::level::critical;

    if (isCritical) { ImGui::PushStyleColor(ImGuiCol_Text, 0xFFFFFFFF); }

    bool dummy;
    renderColumn(0, to_short_c_str(entry.Level), dummy);
    renderColumn(1, entry.Timestamp, options.ShowTimeStamp);
    renderColumn(2, entry.LoggerName, options.ShowLogSource);
    renderColumn(3, entry.Entry, dummy);
    renderColumn(4, entry.FormatSourceLocation(options.SourceLocationRenderStyle), options.ShowSourceLocation);

    ImGui::PopStyleColor(isCritical ? 1 : 0);
    ImGui::PopID();
    ImGui::EndGroup();
}

void LogWindow::SetVisibility(bool visibility) noexcept
{
    m_isVisible = visibility;
    ImGui::SetWindowFocus(s_windowName);
}

}    // namespace Frasy
