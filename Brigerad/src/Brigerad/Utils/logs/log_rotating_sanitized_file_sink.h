/**
 * @file    log_file_sanitizer.sink.h
 * @author  Paul Thomas
 * @date    4/26/2023
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
#ifndef BRIDGERAD_UTILS_LOGS_LOG_ROTATING_SANITIZED_FILE_SINK_H
#define BRIDGERAD_UTILS_LOGS_LOG_ROTATING_SANITIZED_FILE_SINK_H

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <spdlog/details/file_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/details/synchronous_factory.h>
#include <spdlog/sinks/base_sink.h>
#include <string>
#include <utility>

// Imported from rotating_file_sink.h because it is final for some reasons
template<typename Mutex>
class LogRotatingSanitizedFileSink final : public spdlog::sinks::base_sink<Mutex>
{
public:
    LogRotatingSanitizedFileSink(spdlog::filename_t                 base_filename,
                                 std::size_t                        max_size,
                                 std::size_t                        max_files,
                                 bool                               rotate_on_open = false,
                                 const spdlog::file_event_handlers& event_handlers = {})
    : base_filename_(std::move(base_filename)),
      max_size_(max_size),
      max_files_(max_files),
      file_helper_ {event_handlers}
    {
        if (max_size == 0) { spdlog::throw_spdlog_ex("rotating sink constructor: max_size arg cannot be zero"); }

        if (max_files > 200000)
        {
            spdlog::throw_spdlog_ex("rotating sink constructor: max_files arg cannot exceed 200000");
        }
        file_helper_.open(calc_filename(base_filename_, 0));
        current_size_ = file_helper_.size();    // expensive. called only once
        if (rotate_on_open && current_size_ > 0)
        {
            rotate_();
            current_size_ = 0;
        }
    }

    static spdlog::filename_t calc_filename(const spdlog::filename_t& filename, std::size_t index)
    {
        if (index == 0u) { return filename; }

        spdlog::filename_t basename;
        spdlog::filename_t ext;
        std::tie(basename, ext) = spdlog::details::file_helper::split_by_extension(filename);
        return spdlog::fmt_lib::format(SPDLOG_FILENAME_T("{}.{}{}"), basename, index, ext);
    }
    spdlog::filename_t filename()
    {
        std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
        return file_helper_.filename();
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        spdlog::memory_buf_t formatted;
        auto                 container = sanitize_(msg);
        auto                 sanitized = spdlog::details::log_msg(msg);
        sanitized.payload              = std::string_view(container.begin(), container.end());
        spdlog::sinks::base_sink<Mutex>::formatter_->format(sanitized, formatted);
        auto new_size = current_size_ + formatted.size();

        // rotate if the new estimated file size exceeds max size.
        // rotate only if the real size > 0 to better deal with full disk (see issue #2261).
        // we only check the real size when new_size > max_size_ because it is relatively expensive.
        if (new_size > max_size_)
        {
            file_helper_.flush();
            if (file_helper_.size() > 0)
            {
                rotate_();
                new_size = formatted.size();
            }
        }
        file_helper_.write(formatted);
        current_size_ = new_size;
    }
    void flush_() override { file_helper_.flush(); }

private:
    std::string sanitize_(spdlog::details::log_msg msg)
    {
        std::stringstream stream;
        for (const uint8_t c : msg.payload)
        {
            if (std::iscntrl(c) != 0)
            {
                stream << "\\0" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
            }
            else if (c == '\\') { stream << "\\\\"; }
            else if (c == '\"') { stream << "\\\""; }
            else { stream << static_cast<char>(c); }
        }
        return stream.str();
    }

    // Rotate files:
    // log.txt -> log.1.txt
    // log.1.txt -> log.2.txt
    // log.2.txt -> log.3.txt
    // log.3.txt -> delete
    void rotate_()
    {
        using spdlog::details::os::filename_to_str;
        using spdlog::details::os::path_exists;

        file_helper_.close();
        for (auto i = max_files_; i > 0; --i)
        {
            spdlog::filename_t src = calc_filename(base_filename_, i - 1);
            if (!path_exists(src)) { continue; }
            spdlog::filename_t target = calc_filename(base_filename_, i);

            if (!rename_file_(src, target))
            {
                // if failed try again after a small delay.
                // this is a workaround to a windows issue, where very high rotation
                // rates can cause the rename to fail with permission denied (because of antivirus?).
                spdlog::details::os::sleep_for_millis(100);
                if (!rename_file_(src, target))
                {
                    file_helper_.reopen(
                      true);    // truncate the log file anyway to prevent it to grow beyond its limit!
                    current_size_ = 0;
                    spdlog::throw_spdlog_ex("rotating_file_sink: failed renaming " + filename_to_str(src) + " to " +
                                              filename_to_str(target),
                                            errno);
                }
            }
        }
        file_helper_.reopen(true);
    }

    // delete the target if exists, and rename the src file  to target
    // return true on success, false otherwise.
    bool rename_file_(const spdlog::filename_t& src_filename, const spdlog::filename_t& target_filename)
    {
        // try to delete the target file in case it already exists.
        (void)spdlog::details::os::remove(target_filename);
        return spdlog::details::os::rename(src_filename, target_filename) == 0;
    }

    spdlog::filename_t           base_filename_;
    std::size_t                  max_size_;
    std::size_t                  max_files_;
    std::size_t                  current_size_;
    spdlog::details::file_helper file_helper_;
};

using LogRotatingSanitizedFileSinkMt = LogRotatingSanitizedFileSink<std::mutex>;
using LogRotatingSanitizedFileSinkSt = LogRotatingSanitizedFileSink<spdlog::details::null_mutex>;

//
// factory functions
//

template<typename Factory = spdlog::synchronous_factory>
inline std::shared_ptr<spdlog::logger> rotating_logger_mt(const std::string&                 logger_name,
                                                          const spdlog::filename_t&          filename,
                                                          size_t                             max_file_size,
                                                          size_t                             max_files,
                                                          bool                               rotate_on_open = false,
                                                          const spdlog::file_event_handlers& event_handlers = {})
{
    return Factory::template create<LogRotatingSanitizedFileSink>(
      logger_name, filename, max_file_size, max_files, rotate_on_open, event_handlers);
}

template<typename Factory = spdlog::synchronous_factory>
inline std::shared_ptr<spdlog::logger> rotating_logger_st(const std::string&                 logger_name,
                                                          const spdlog::filename_t&          filename,
                                                          size_t                             max_file_size,
                                                          size_t                             max_files,
                                                          bool                               rotate_on_open = false,
                                                          const spdlog::file_event_handlers& event_handlers = {})
{
    return Factory::template create<LogRotatingSanitizedFileSink>(
      logger_name, filename, max_file_size, max_files, rotate_on_open, event_handlers);
}

#endif    // BRIDGERAD_UTILS_LOGS_LOG_ROTATING_SANITIZED_FILE_SINK_H
