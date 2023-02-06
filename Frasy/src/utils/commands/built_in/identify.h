/**
 * @file    info.h
 * @author  Samuel Martel
 * @date    2022-12-14
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

#ifndef FRASY_INSTRUMENTATION_CARD_INFO_H
#define FRASY_INSTRUMENTATION_CARD_INFO_H

#include "utils/commands/command.h"
#include "utils/misc/deserializer.h"

#include <array>
#include <cstdint>
#include <spdlog/fmt/fmt.h>
#include <string>

namespace Frasy::Actions::Identify
{

struct Reply
{
    std::array<uint32_t, 4>  Uuid              = {};
    uint8_t                  Id                = 0;
    std::array<uint8_t, 32>  Version           = {};
    std::array<uint8_t, 32>  PrjName           = {};
    std::array<uint8_t, 16>  BuildTime         = {};
    std::array<uint8_t, 16>  BuildDate         = {};
    std::vector<std::string> SupportedCommands = {};
};
static_assert(std::same_as<decltype(Deserialize<Reply>(nullptr, nullptr)), Reply>);

using CommandInfo = Commands::GenericCommand<0x8000, void, Reply>;

struct PrettyInstrumentationCardInfo
{
    std::string              Uuid;
    uint8_t                  Id = 0;
    std::string              Version;
    std::string              PrjName;
    std::string              Built;
    std::vector<std::string> SupportedCommands;

    PrettyInstrumentationCardInfo() = default;
    PrettyInstrumentationCardInfo(const Reply& o) : Id(o.Id), SupportedCommands(o.SupportedCommands)
    {
        Uuid    = fmt::format("{:08X}", fmt::join(o.Uuid, ""));
        Version = std::string {o.Version.begin(), o.Version.end()};
        PrjName = std::string {o.PrjName.begin(), o.PrjName.end()};
        Built   = fmt::format("{} - {}",
                            std::string {o.BuildDate.begin(), o.BuildDate.end()}.c_str(),
                            std::string {o.BuildTime.begin(), o.BuildTime.end()}.c_str());
    }
};

}    // namespace Frasy::Actions::Identify

#endif    // FRASY_INSTRUMENTATION_CARD_INFO_H
