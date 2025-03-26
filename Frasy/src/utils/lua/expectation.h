/**
 * @file    expectation.h
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

#ifndef FRASY_SRC_UTILS_LUA_EXPECTATION_H
#define FRASY_SRC_UTILS_LUA_EXPECTATION_H

#include <sol/sol.hpp>
#include <string>

namespace Frasy::Lua {

/**
 * Simplified version of Expectation from Lua
 * It is currently meant to be used for displaying them
 * on the GUI
 * It should be remade once we do the full C++ port of the orchestrator
 */
struct Expectation {
    enum Method {
        toBeTrue,
        toBeFalse,
        toBeEqual,
        toBeNear,
        toBeInRange,
        toBeInPercentage,
        toBeGreater,
        toBeGreaterOrEqual,
        toBeLesser,
        toBeLesserOrEqual,
        toBeType,
        toMatch
    };

    static Expectation fromTable(const sol::table& table);

    std::string name;
    Method      method;
    std::string measure;
    std::string min;
    std::string max;
    bool        pass;
};

}    // namespace Frasy::Lua

#endif    // FRASY_SRC_UTILS_LUA_EXPECTATION_H
