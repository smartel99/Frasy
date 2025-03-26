/**
 * @file    expectation.cpp
 * @author  Paul Thomas
 * @date    3/25/2025
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

#include "expectation.h"

namespace Frasy::Lua {

namespace {

std::string solObjectToString(const sol::object& object)
{
    const auto type = object.get_type();
    if (type == sol::type::string) { return object.as<std::string>(); }
    if (type == sol::type::number) {
        if (object.is<int>()) { return std::to_string(object.as<int>()); }
        if (object.is<double>()) { return std::to_string(object.as<double>()); }
        throw std::runtime_error("Invalid number type");
    }
    if (type == sol::type::boolean) { return object.as<bool>() ? "True" : "False"; }
    throw std::runtime_error("Invalid type");
}

}    // namespace

Expectation Expectation::fromTable(const sol::table& table)
{
    Expectation expectation;
    expectation.name   = table["name"].get_or<std::string>("");
    expectation.pass   = table["pass"].get<bool>();
    if (table["inverted"].get<bool>()) { expectation.pass = !expectation.pass; }
    if (const auto method = table["method"].get_or<std::string>(""); method == "ToBeTrue") {
        expectation.method  = toBeTrue;
        expectation.max     = "True";
        expectation.min     = "True";
        expectation.measure = solObjectToString(table["value"]);
    }
    else if (method == "ToBeFalse") {
        expectation.method  = toBeFalse;
        expectation.max     = "False";
        expectation.min     = "False";
        expectation.measure = solObjectToString(table["value"]);
    }
    else if (method == "ToBeEqual") {
        expectation.method  = toBeEqual;
        expectation.max     = solObjectToString(table["expected"]);
        expectation.min     = solObjectToString(table["expected"]);
        expectation.measure = solObjectToString(table["value"]);
    }
    else if (method == "ToBeNear") {
        expectation.method  = toBeNear;
        expectation.max     = solObjectToString(table["min"]);
        expectation.min     = solObjectToString(table["max"]);
        expectation.measure = solObjectToString(table["value"]);
    }
    else if (method == "ToBeInRange") {
        expectation.method  = toBeInRange;
        expectation.max     = solObjectToString(table["max"]);
        expectation.min     = solObjectToString(table["min"]);
        expectation.measure = solObjectToString(table["value"]);
    }
    else if (method == "ToBeInPercentage") {
        expectation.method  = toBeInPercentage;
        expectation.max     = solObjectToString(table["max"]);
        expectation.min     = solObjectToString(table["min"]);
        expectation.measure = solObjectToString(table["value"]);
    }
    else if (method == "ToBeGreater") {
        expectation.method  = toBeGreater;
        expectation.max     = "N/A";
        expectation.min     = solObjectToString(table["min"]);
        expectation.measure = solObjectToString(table["value"]);
    }
    else if (method == "ToBeGreaterOrEqual") {
        expectation.method  = toBeGreaterOrEqual;
        expectation.max     = "N/A";
        expectation.min     = solObjectToString(table["min"]);
        expectation.measure = solObjectToString(table["value"]);
    }
    else if (method == "ToBeLesser") {
        expectation.method  = toBeLesser;
        expectation.max     = solObjectToString(table["max"]);
        expectation.min     = "N/A";
        expectation.measure = solObjectToString(table["value"]);
    }
    else if (method == "ToBeLesserOrEqual") {
        expectation.method  = toBeLesserOrEqual;
        expectation.max     = solObjectToString(table["max"]);
        expectation.min     = "N/A";
        expectation.measure = solObjectToString(table["value"]);
    }
    else if (method == "ToBeType") {
        expectation.method  = toBeType;
        expectation.max     = solObjectToString(table["expected"]);
        expectation.min     = solObjectToString(table["expected"]);
        expectation.measure = solObjectToString(table["value"]);
    }
    else if (method == "ToMatch") {
        expectation.method  = toMatch;
        expectation.max     = solObjectToString(table["expected"]);
        expectation.min     = solObjectToString(table["expected"]);
        expectation.measure = solObjectToString(table["value"]);
    }
    return expectation;
}

}    // namespace Frasy::Lua
