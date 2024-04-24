/*
 * CAN module object for Linux socketCAN Error handling.
 *
 * @file        CO_error.c
 * @author      Martin Wagner
 * @copyright   2018 - 2020 Neuberger Gebaeudeautomation GmbH
 *
 *
 * This file is part of CANopenNode, an opensource CANopen Stack.
 * Project home page is <https://github.com/CANopenNode/CANopenNode>.
 * For more information on CANopen see <http://www.can-cia.org/>.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "CO_error.h"

#include <Brigerad.h>

#include <cstdarg>
#include <cstdio>

#ifdef __cplusplus
extern "C" {
#endif

namespace {
char buffer[1024] = {};

spdlog::level::level_enum intToLevel(int level)
{
    switch (level) {
        case LOG_EMERG: return spdlog::level::level_enum::critical;
        case LOG_ALERT: return spdlog::level::level_enum::critical;
        case LOG_CRIT: return spdlog::level::level_enum::critical;
        case LOG_ERR: return spdlog::level::level_enum::err;
        case LOG_WARNING: return spdlog::level::level_enum::warn;
        case LOG_NOTICE: return spdlog::level::level_enum::info;
        case LOG_INFO: return spdlog::level::level_enum::info;
        case LOG_DEBUG: return spdlog::level::level_enum::debug;
        default: return spdlog::level::level_enum::debug;
    }
}
}    // namespace

void log_printf(int level, const char* fmt, ...)
{
    std::va_list args;
    va_start(args, fmt);
    std::vsnprintf(&buffer[0], sizeof(buffer), fmt, args);
    va_end(args);

    BR_LOG("CANopen", intToLevel(level), "{}", &buffer[0]);
}

#ifdef __cplusplus
}
#endif
