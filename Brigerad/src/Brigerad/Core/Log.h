#pragma once
#include "Core.h"

#include <atomic>
#include <cctype>
#include <memory>
#include <ranges>
#include <source_location>
#include <spdlog/sinks/dist_sink.h>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

namespace Brigerad
{
class BRIGERAD_API Log
{
public:
    using Logger = std::shared_ptr<spdlog::logger>;

    static void Init(bool useStderr = false, bool silent = false);

    static Logger GetCoreLogger() { return GetLogger(s_coreLoggerName); }
    static Logger GetClientLogger() { return GetLogger(s_clientLoggerName); }
    static Logger GetLuaLogger() { return GetLogger(s_luaLoggerName); }

    using LoggerMap = std::unordered_map<std::string, Logger>;

    static Logger GetLogger(const std::string& name)    // by value now
    {
        auto current = s_loggers.load(std::memory_order_acquire);
        if (auto it = current->find(name); it != current->end()) { return it->second; }

        auto logger = std::make_shared<spdlog::logger>(name, s_sinks);
        logger->set_level(s_defaultLevel);    // fully set up before anyone can see it
        while (true)
        {
            auto next = std::make_shared<LoggerMap>(*current);
            next->emplace(name, logger);
            if (s_loggers.compare_exchange_weak(current, std::move(next), std::memory_order_acq_rel)) { return logger; }
            // CAS failed: `current` now holds the newer map. Another thread may have added this name.
            if (auto it = current->find(name); it != current->end()) { return it->second; }
        }
    }

    static std::shared_ptr<const LoggerMap> GetLoggers() { return s_loggers.load(); }

    static spdlog::level::level_enum GetLoggerLevel(const std::string& name) { return GetLogger(name)->level(); }

    static void SetLoggerLevel(const std::string& name, spdlog::level::level_enum level)
    {
        GetLogger(name)->set_level(level);
    }

    static void AddSink(const spdlog::sink_ptr& sink);

    static void RemoveSink(const spdlog::sink_ptr& sink);

    static spdlog::source_loc FormatSourceLocation(const std::source_location& loc);

    static constexpr const char* s_coreLoggerName   = "BRIGERAD";
    static constexpr const char* s_clientLoggerName = "APP";
    static constexpr const char* s_luaLoggerName    = "LUA";
    static constexpr spdlog::level::level_enum s_defaultLevel     = spdlog::level::info;

private:
    static inline std::shared_ptr<spdlog::sinks::dist_sink_mt>                     s_sinks   = nullptr;
    static inline std::atomic<std::shared_ptr<const LoggerMap>>                    s_loggers {std::make_shared<const LoggerMap>()};

    static spdlog::file_event_handlers s_eventHandlers;

};

}    // namespace Brigerad

#define BR_LOG(logger, level, msg, ...)      \
    ::Brigerad::Log::GetLogger(logger)->log( \
      ::Brigerad::Log::FormatSourceLocation(std::source_location::current()), level, msg __VA_OPT__(, ) __VA_ARGS__)

#define BR_LOG_TRACE(logger, ...)    BR_LOG(logger, ::spdlog::level::trace, __VA_ARGS__)
#define BR_LOG_DEBUG(logger, ...)    BR_LOG(logger, ::spdlog::level::debug, __VA_ARGS__)
#define BR_LOG_INFO(logger, ...)     BR_LOG(logger, ::spdlog::level::info, __VA_ARGS__)
#define BR_LOG_WARN(logger, ...)     BR_LOG(logger, ::spdlog::level::warn, __VA_ARGS__)
#define BR_LOG_ERROR(logger, ...)    BR_LOG(logger, ::spdlog::level::err, __VA_ARGS__)
#define BR_LOG_CRITICAL(logger, ...) BR_LOG(logger, ::spdlog::level::critical, __VA_ARGS__)

// Core Log Macros.
#define BR_CORE_TRACE(...)    BR_LOG_TRACE(::Brigerad::Log::s_coreLoggerName, __VA_ARGS__)
#define BR_CORE_DEBUG(...)    BR_LOG_DEBUG(::Brigerad::Log::s_coreLoggerName, __VA_ARGS__)
#define BR_CORE_INFO(...)     BR_LOG_INFO(::Brigerad::Log::s_coreLoggerName, __VA_ARGS__)
#define BR_CORE_WARN(...)     BR_LOG_WARN(::Brigerad::Log::s_coreLoggerName, __VA_ARGS__)
#define BR_CORE_ERROR(...)    BR_LOG_ERROR(::Brigerad::Log::s_coreLoggerName, __VA_ARGS__)
#define BR_CORE_CRITICAL(...) BR_LOG_CRITICAL(::Brigerad::Log::s_coreLoggerName, __VA_ARGS__)

// Client Log Macros.
#define BR_APP_TRACE(...)    BR_LOG_TRACE(::Brigerad::Log::s_clientLoggerName, __VA_ARGS__)
#define BR_APP_DEBUG(...)    BR_LOG_DEBUG(::Brigerad::Log::s_clientLoggerName, __VA_ARGS__)
#define BR_APP_INFO(...)     BR_LOG_INFO(::Brigerad::Log::s_clientLoggerName, __VA_ARGS__)
#define BR_APP_WARN(...)     BR_LOG_WARN(::Brigerad::Log::s_clientLoggerName, __VA_ARGS__)
#define BR_APP_ERROR(...)    BR_LOG_ERROR(::Brigerad::Log::s_clientLoggerName, __VA_ARGS__)
#define BR_APP_CRITICAL(...) BR_LOG_CRITICAL(::Brigerad::Log::s_clientLoggerName, __VA_ARGS__)

// Lua Log Macros.
#define BR_LUA_TRACE(...)    BR_LOG_TRACE(::Brigerad::Log::s_luaLoggerName, __VA_ARGS__)
#define BR_LUA_DEBUG(...)    BR_LOG_DEBUG(::Brigerad::Log::s_luaLoggerName, __VA_ARGS__)
#define BR_LUA_INFO(...)     BR_LOG_INFO(::Brigerad::Log::s_luaLoggerName, __VA_ARGS__)
#define BR_LUA_WARN(...)     BR_LOG_WARN(::Brigerad::Log::s_luaLoggerName, __VA_ARGS__)
#define BR_LUA_ERROR(...)    BR_LOG_ERROR(::Brigerad::Log::s_luaLoggerName, __VA_ARGS__)
#define BR_LUA_CRITICAL(...) BR_LOG_CRITICAL(::Brigerad::Log::s_luaLoggerName, __VA_ARGS__)
