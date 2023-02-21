/**
 * @file    reply.cpp
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

#include "reply.h"

#include "utils/misc/deserializer.h"

#include <iomanip>
#include <sstream>

namespace Frasy::Actions::Identify
{
type_id_t Reply::Manager::id = 0;

static std::string PrintUUID(std::array<uint32_t, Reply::uuid_size> uuid);
static std::string PrintVersion(std::array<uint8_t, Reply::version_size> version);
static std::string PrintName(std::array<uint8_t, Reply::prj_name_size> name);
static std::string PrintBuildTime(std::array<uint8_t, Reply::build_time_size> build_time);
static std::string PrintBuildDate(std::array<uint8_t, Reply::build_date_size> build_date);

Reply::operator std::string() const
{
    return "{" + PrintUUID(Uuid) + ", " +        //
           std::to_string(Id) + ", " +           //
           PrintVersion(Version) + ", " +        //
           PrintName(PrjName) + ", " +           //
           PrintBuildTime(BuildTime) + ", " +    //
           PrintBuildDate(BuildDate) + "}";
}

std::string PrintUUID(std::array<uint32_t, Reply::uuid_size> uuid)
{
    std::stringstream ss;
    for (auto e : uuid) { ss << std::setw(sizeof(uint32_t) * 2) << std::setfill('0') << std::setbase(16) << e; }
    return ss.str();
}

std::string PrintVersion(std::array<uint8_t, Reply::version_size> version)
{
    std::stringstream ss;
    for (auto e : version) { ss << e << "."; }
    std::string s = ss.str();
    s.pop_back();
    return s;
}

std::string PrintName(std::array<uint8_t, Reply::prj_name_size> name)
{
    return {name.begin(), name.end()};
}
std::string PrintBuildTime(std::array<uint8_t, Reply::build_time_size> time)
{
    return {time.begin(), time.end()};
}
std::string PrintBuildDate(std::array<uint8_t, Reply::build_date_size> date)
{
    return {date.begin(), date.end()};
}

static_assert(requires {
                  {
                      Deserialize<Reply>(nullptr, nullptr)
                  } -> std::same_as<Reply>;
              });

}    // namespace Frasy::Actions::Identify
