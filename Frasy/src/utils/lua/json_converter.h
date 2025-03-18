/**
 * @file    json_converter.h
 * @author  Paul Thomas
 * @date    3/17/2025
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
 * not, see <https://www.gnu.org/licenses/>.
 */

#ifndef JSON_CONVERTER_H
#define JSON_CONVERTER_H

#include <sol/sol.hpp>
#include <json.hpp>

namespace Frasy::Lua {

nlohmann::json tableToJson(const sol::table& table);
sol::table jsonToTable(const sol::table& table, const nlohmann::json& json);

}

#endif //JSON_CONVERTER_H
