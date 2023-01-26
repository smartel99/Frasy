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

#include "utils/misc/deserializer.h"

#include <array>
#include <cstdint>
#include <spdlog/fmt/fmt.h>
#include <string>

namespace Frasy
{

struct InstrumentationCardInfo
{
    std::array<uint8_t, 16> Uuid                   = {};
    uint8_t                 Id                     = 0;
    std::array<uint8_t, 32> Version                = {};
    std::array<uint8_t, 32> PrjName                = {};
    std::array<uint8_t, 16> BuildTime              = {};
    std::array<uint8_t, 16> BuildDate              = {};
    uint16_t                SupportedCommandsCount = 0;
};
static_assert(std::same_as<decltype(Deserialize<InstrumentationCardInfo>(nullptr, nullptr)), InstrumentationCardInfo>);

struct PrettyInstrumentationCardInfo
{
    std::string Uuid;
    uint8_t     Id = 0;
    std::string Version;
    std::string PrjName;
    std::string Built;
    uint16_t    SupportedCommandsCount = 0;

    PrettyInstrumentationCardInfo() = default;
    PrettyInstrumentationCardInfo(const InstrumentationCardInfo& o)
    : Id(o.Id), SupportedCommandsCount(o.SupportedCommandsCount)
    {
        auto toStr = [](auto v) -> std::string { return fmt::format("{:02X}", fmt::join(v, "")); };
        Uuid       = fmt::format("{:02X}", fmt::join(o.Uuid, ""));
        Version    = std::string {o.Version.begin(), o.Version.end()};
        PrjName    = std::string {o.PrjName.begin(), o.PrjName.end()};
        Built      = fmt::format("{} - {}",
                            std::string {o.BuildDate.begin(), o.BuildDate.end()}.c_str(),
                            std::string {o.BuildTime.begin(), o.BuildTime.end()}.c_str());
    }
};

}    // namespace Frasy

#endif    // FRASY_INSTRUMENTATION_CARD_INFO_H
