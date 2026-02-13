/**
 * @file    log_window_sink.h
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

#ifndef FRASY_UTILS_LOG_WINDOW_SINK_H
#define FRASY_UTILS_LOG_WINDOW_SINK_H

#include "log_entry.h"
#include "spdlog/details/circular_q.h"
#include "spdlog/sinks/base_sink.h"

#include <array>
#include <map>
#include <mutex>
#include <string>
#include <string_view>

namespace Frasy {

class LogWindowSink : public spdlog::sinks::base_sink<std::mutex> {};

class LogWindowMultiSink : public LogWindowSink {
public:
    using LoggerMap = std::map<std::string_view, spdlog::details::circular_q<LogEntry>>;

    explicit LogWindowMultiSink(size_t max) noexcept : m_maxLen {max} {}

    [[nodiscard]] const LoggerMap& GetLoggers() const noexcept { return m_loggers; }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        if (!m_loggers.contains(msg.logger_name)) {
            // New logger!
            m_loggers[msg.logger_name] = spdlog::details::circular_q<LogEntry>(m_maxLen);
        }
        m_loggers[msg.logger_name].push_back({msg.level,
                                              std::string(msg.logger_name),
                                              msg.source.filename,
                                              msg.source.funcname,
                                              msg.source.line,
                                              std::string(msg.payload),
                                              std::format("{}", msg.time)});
    }

    void flush_() override {}

private:
    size_t    m_maxLen = 0;
    LoggerMap m_loggers;

    friend class LogWindowSink;
};

class LogWindowSingleSink : public LogWindowSink {
public:
    explicit LogWindowSingleSink(size_t max) noexcept : m_maxLen {max}, m_entries(m_maxLen) {}

    [[nodiscard]] const spdlog::details::circular_q<LogEntry>& GetEntries() const noexcept { return m_entries; }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        m_entries.push_back({msg.level,
                             std::string(msg.logger_name),
                             msg.source.filename,
                             msg.source.funcname,
                             msg.source.line,
                             std::string(msg.payload),
                             std::format("{}", msg.time)});
    }

    void flush_() override {}

private:
    size_t                                m_maxLen = 0;
    spdlog::details::circular_q<LogEntry> m_entries;
};


}    // namespace Frasy
#endif    // FRASY_UTILS_LOG_WINDOW_SINK_H
