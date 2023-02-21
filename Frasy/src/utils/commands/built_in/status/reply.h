/**
 * @file    reply.h
 * @author  Paul Thomas
 * @date    2023-02-16
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
#ifndef INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_STATUS_REPLY_H
#define INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_STATUS_REPLY_H

#include "../../type/fundamental.h"
#include "../../type/type_id_t.h"
#include "error_code.h"

#include <string>
#include <string_view>
namespace Frasy::Actions::Status
{
struct Reply
{
    struct Manager
    {
        static constexpr std::string_view name = "Status::Reply";
        static constexpr std::string_view description =
          "Report the current state of the board or report errors from commands";
        static type_id_t id;

        // Fields
        struct Message
        {
            static constexpr std::string_view name        = "Message";
            static constexpr std::string_view description = "String representation of the status";
            static constexpr type_id_t        type        = static_cast<type_id_t>(Type::Fundamental::E::String);
        };
        struct Code
        {
            static constexpr std::string_view name        = "Code";
            static constexpr std::string_view description = "Numeric code of the status";
            static type_id_t                  type() { return ErrorCode::Manager::id; }
        };
    };

    std::string  Message;
    ErrorCode::E Code = ErrorCode::E::NoError;

    explicit operator std::string() const
    {
        return "{" + Message + ", " + std::to_string(static_cast<uint32_t>(Code)) + "}";
    }
};
}    // namespace Frasy::Actions::Status

#endif    // INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_STATUS_REPLY_H
