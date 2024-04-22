/**
 * CANopenNode Linux socketCAN Error handling.
 *
 * @file        CO_error.h
 * @ingroup     CO_socketCAN_ERROR
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


#ifndef CO_ERROR_H
#define CO_ERROR_H

#include <errno.h>
#include <stdint.h>
#include <string.h>

#if __has_include("CO_error_custom.h")
#    include "CO_error_custom.h"
#else
#    include "CO_error_msgs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup CO_socketCAN_ERROR CAN errors & Log
 * CANopen Errors and System message log
 *
 * @ingroup CO_socketCAN
 * @{
 */

#define LOG_EMERG   0
#define LOG_ALERT   1
#define LOG_CRIT    2
#define LOG_ERR     3
#define LOG_WARNING 4
#define LOG_NOTICE  5
#define LOG_INFO    6
#define LOG_DEBUG   7

/**
 * Message logging function.
 *
 * Function must be defined by application. It should record log message to some
 * place, for example syslog() call in Linux or logging functionality in
 * CANopen gateway @ref CO_CANopen_309_3.
 *
 * By default system stores messages in /var/log/syslog file.
 * Log can optionally be configured before, for example to filter out less
 * critical errors than LOG_NOTICE, specify program name, print also process PID
 * and print also to standard error, set 'user' type of program, use:
 * @code
 * setlogmask (LOG_UPTO (LOG_NOTICE));
 * openlog ("exampleprog", LOG_PID | LOG_PERROR, LOG_USER);
 * @endcode
 *
 * @param priority one of LOG_EMERG, LOG_ALERT, LOG_CRIT, LOG_ERR, LOG_WARNING,
 *                 LOG_NOTICE, LOG_INFO, LOG_DEBUG
 * @param format format string as in printf
 */
void log_printf(int priority, const char* format, ...);


/** @} */

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /* CO_ERROR_H */
