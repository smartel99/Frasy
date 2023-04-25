/**
 * @file    error_codes.h
 * @author  Paul Thomas
 * @date    2023-02-07
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
#ifndef INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_STATUS_ERROR_CODE_H
#define INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_STATUS_ERROR_CODE_H

#include "../../type/type_id_t.h"

#include <cstdint>
#include <string_view>

namespace Frasy::Actions::Status
{
struct ErrorCode
{
    enum class E : uint32_t
    {
        NoError,
        NotFound,
        InvalidArgument,
        NoResources,
        GenericError,
        NotSupported,
        PacketError,
    };

    struct Manager
    {
        static constexpr std::string_view name        = "ErrorCode";
        static constexpr std::string_view description = "Various code representing the status of a command";
        static type_id_t                  id;

        // Values
        struct NoError
        {
            static constexpr std::string_view name        = "NoError";
            static constexpr std::string_view description = "Reported when board is working properly";
            static constexpr uint32_t         value       = static_cast<uint32_t>(E::NoError);
        };
        struct NotFound
        {
            static constexpr std::string_view name        = "NotFound";
            static constexpr std::string_view description = "Reported when board could not found requested value";
            static constexpr uint32_t         value       = static_cast<uint32_t>(E::NotFound);
        };
        struct InvalidArgument
        {
            static constexpr std::string_view name = "InvalidArgument";
            static constexpr std::string_view description =
              "Reported when an argument passed to a command is not valid";
            static constexpr uint32_t value = static_cast<uint32_t>(E::InvalidArgument);
        };
        struct NoResources
        {
            static constexpr std::string_view name = "NoResources";
            static constexpr std::string_view description =
              "Reported when there are no available resources to execute the requested command";
            static constexpr uint32_t value = static_cast<uint32_t>(E::NoResources);
        };
        struct GenericError
        {
            static constexpr std::string_view name        = "GenericError";
            static constexpr std::string_view description = "Reported when undocumented error arise";
            static constexpr uint32_t         value       = static_cast<uint32_t>(E::GenericError);
        };
        struct NotSupported
        {
            static constexpr std::string_view name        = "NotSupported";
            static constexpr std::string_view description = "Reported when a requested feature is not supported";
            static constexpr uint32_t         value       = static_cast<uint32_t>(E::NotSupported);
        };
        struct PacketError
        {
            static constexpr std::string_view name        = "PacketError";
            static constexpr std::string_view description = "Reported when a packet fails to be parsed";
            static constexpr uint32_t         value       = static_cast<uint32_t>(E::PacketError);
        };
    };

    static constexpr std::string_view ToStr(E v)
    {
        switch (v)
        {
            case E::NoError: return Manager::NoError::name;
            case E::NotFound: return Manager::NotFound::name;
            case E::InvalidArgument: return Manager::InvalidArgument::name;
            case E::NoResources: return Manager::NoResources::name;
            case E::GenericError: return Manager::GenericError::name;
            case E::NotSupported: return Manager::NotSupported::name;
            case E::PacketError: return Manager::PacketError::name;
            default: return "Unknown error";
        }
    }
};
}    // namespace Frasy::Actions::Status

#endif    // INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_STATUS_ERROR_CODE_H
