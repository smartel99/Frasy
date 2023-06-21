#include "Log.h"

#include "../Utils/logs/log_rotating_sanitized_file_sink.h"
#include <spdlog/sinks/stdout_color_sinks.h>

#include <spdlog/fmt/chrono.h>

#include <algorithm>
#include <fstream>

namespace Brigerad
{
spdlog::file_event_handlers Log::s_eventHandlers = []()
{
    spdlog::file_event_handlers handlers;

    handlers.before_open = []([[maybe_unused]] const spdlog::filename_t& path) {

    };

    handlers.after_open = [=]([[maybe_unused]] const spdlog::filename_t& path, std::FILE* file)
    {
        spdlog::source_loc loc =
          Brigerad::Log::FormatSourceLocation(std::source_location::current());
        std::string header = fmt::format(
          "[{{"
          "\"timestamp\": \"{:%Y-%m-%dT%T.000Z}\","
          "\"level\": \"trace\","
          "\"from\": \"system\","
          "\"message\": \"Initialized Log File\","
          "\"where\": {{"
          "\"file\": \"{}\","
          "\"line\": \"{}\","
          "\"function\": \"{}\""
          "}}"
          "}}\n",
          std::chrono::system_clock::now(),
          loc.filename,
          loc.line,
          loc.funcname);

        std::fputs(header.c_str(), file);
    };

    handlers.before_close = [=]([[maybe_unused]] const spdlog::filename_t& path, std::FILE* file)
    { std::fputc(']', file); };

    handlers.after_close = []([[maybe_unused]] const spdlog::filename_t& path) {};

    return handlers;
}();

void Log::Init()
{
    // %T -> Timestamp
    // %n -> Name of the logger
    // %v -> Message
    spdlog::set_pattern("%^[%T] %n: %v%$");
    s_sinks = std::make_shared<decltype(s_sinks)::element_type>();

    auto stdSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    stdSink->set_pattern("%^%L[%T.%e] %n: %v%$");
    s_sinks->add_sink(stdSink);

    static constexpr size_t maxSize = 1024 * 1024 * 1;    //!< 1MB max
    auto rotatingFileSink           = std::make_shared<LogRotatingSanitizedFileSinkMt>(
      "logs/frasy/log.json", maxSize, 5, true, s_eventHandlers);

    // clang-format off
    static constexpr const char* pattern =
",{"
    "\"timestamp\": \"%Y-%m-%dT%T.%eZ\","
    "\"level\": \"%l\","
    "\"from\": \"%n\","
    "\"message\": \"%v\","
    "\"where\": {"
      "\"file\": \"%g\","
      "\"line\": \"%#\","
      "\"function\": \"%!\""
    "}"
"}";
    // clang-format on
    rotatingFileSink->set_pattern(pattern);
    s_sinks->add_sink(rotatingFileSink);

    auto logger = GetLogger(s_coreLoggerName);
    logger->set_level(spdlog::level::trace);

    logger = GetLogger(s_clientLoggerName);
    logger->set_level(spdlog::level::trace);

    logger = GetLogger(s_luaLoggerName);
    logger->set_level(spdlog::level::trace);
}

void Log::AddSink(const spdlog::sink_ptr& sink)
{
    s_sinks->add_sink(sink);
    BR_CORE_TRACE("Added sink to loggers");
}

void Log::RemoveSink(const spdlog::sink_ptr& sink)
{
    s_sinks->remove_sink(sink);
    BR_CORE_TRACE("Removed sink from loggers");
}

spdlog::source_loc Log::FormatSourceLocation(const std::source_location& loc)
{
    static std::string filename;
    filename = loc.file_name();

    std::ranges::replace_if(
      filename, [](auto c) { return c == '\\'; }, '/');

    return {filename.c_str(), static_cast<int>(loc.line()), loc.function_name()};
}
}    // namespace Brigerad
