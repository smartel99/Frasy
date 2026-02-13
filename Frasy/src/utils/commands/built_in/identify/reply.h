/**
 * @file    reply.h
 * @author  Paul Thomas
 * @date    2023-02-15
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
#ifndef INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_IDENTIFY_REPLY_H
#define INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_IDENTIFY_REPLY_H

#include "../../type/count.h"
#include "../../type/fundamental.h"
#include "utils/misc/deserializer.h"
#include "utils/misc/serializer.h"

#include <array>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>
#include <format>

namespace Frasy::Actions::Identify
{
struct Reply
{
    static constexpr uint16_t uuid_size       = Type::ARRAY(4);
    static constexpr uint16_t version_size    = Type::ARRAY(32);
    static constexpr uint16_t prj_name_size   = Type::ARRAY(32);
    static constexpr uint16_t build_time_size = Type::ARRAY(16);
    static constexpr uint16_t build_date_size = Type::ARRAY(16);

    std::array<uint32_t, uuid_size>      Uuid      = {};
    uint8_t                              Id        = 0;
    std::array<uint8_t, version_size>    Version   = {};
    std::array<uint8_t, prj_name_size>   PrjName   = {};
    std::array<uint8_t, build_time_size> BuildTime = {};
    std::array<uint8_t, build_date_size> BuildDate = {};

    explicit operator std::string() const;
    struct Manager;
};

struct Info
{
    std::string Uuid    = {};
    uint8_t     Id      = 0;
    std::string Version = {};
    std::string PrjName = {};
    std::string Built   = {};

    Info() = default;
    explicit Info(const Reply& reply)
    {
        Uuid    = std::format("{::08X}", reply.Uuid);
        Version = std::string {reply.Version.begin(), reply.Version.end()};
        PrjName = std::string {reply.PrjName.begin(), reply.PrjName.end()};
        Built   = std::format("{} - {}",
                            std::string {reply.BuildDate.begin(), reply.BuildDate.end()}.c_str(),
                            std::string {reply.BuildTime.begin(), reply.BuildTime.end()}.c_str());
    }
};    // namespace Frasy::Actions::Identify

using Type::Fundamental;
struct Reply::Manager
{
    static constexpr std::string_view name        = "IdentifyReply";
    static constexpr std::string_view description = "Provide basic information of the board";
    static type_id_t                  id;

    // fields
    struct Uuid
    {
        static constexpr std::string_view name        = "UUID";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::UInt32);
        static constexpr uint16_t         count       = uuid_size;
        static constexpr std::string_view description = "Name of the command. Can be used to invoke it";
    };
    struct Id
    {
        static constexpr std::string_view name        = "ID";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
        static constexpr std::string_view description = "Tell in which slot of the motherboard, this board is put";
    };
    struct Version
    {
        static constexpr std::string_view name        = "Version";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
        static constexpr uint16_t         count       = version_size;
        static constexpr std::string_view description = "Version of the firmware";
    };
    struct PrjName
    {
        static constexpr std::string_view name        = "Project Name";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
        static constexpr uint16_t         count       = prj_name_size;
        static constexpr std::string_view description = "Name of the firmware";
    };
    struct BuildTime
    {
        static constexpr std::string_view name        = "Build Time";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
        static constexpr uint16_t         count       = build_time_size;
        static constexpr std::string_view description = "Time when the firmware was compiled";
    };
    struct BuildDate
    {
        static constexpr std::string_view name        = "Build Date";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
        static constexpr uint16_t         count       = build_date_size;
        static constexpr std::string_view description = "Date when the firmware was compiled";
    };
};

static_assert(requires {
    {
        Deserialize<Reply>(nullptr, nullptr)
    } -> std::same_as<Reply>;
});
static_assert(requires { Serialize(Reply {}); });

}    // namespace Frasy::Actions::Identify

#endif    // INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_BUILT_IN_IDENTIFY_REPLY_H
