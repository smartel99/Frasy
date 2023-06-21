/**
 * @file    response.h
 * @author  Samuel Martel
 * @date    2022-10-11
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


#ifndef GUARD_NILAI_INTERFACES_COMMANDS_RESPONSE_H
#define GUARD_NILAI_INTERFACES_COMMANDS_RESPONSE_H


#include "../misc/deserializer.h"

#include <string>
#include <type_traits>
#include <vector>


namespace Frasy::Commands
{
template<class Class, typename = void>
struct CommandResponseSize
{
    static constexpr size_t value = 0;
};


template<class Class>
struct CommandResponseSize<Class, std::void_t<decltype(sizeof(Class))>>
{
    static constexpr size_t value = sizeof(Class);
};

static_assert(CommandResponseSize<void>::value == 0);
static_assert(CommandResponseSize<int>::value == sizeof(int));

/**
 * Checks if the command requires a response to be received. This is done at compile time through
 * the two definitions of the functions, each having different specialization.
 *
 * If the command has a response type defined as non-void, the more specialized version returning
 * true will be invoked. Otherwise, the more generic version returning false will be invoked.
 * @tparam T
 * @return
 */
template<typename T>
consteval bool CommandNeedsResponse()
{
    return requires { typename T::response_type; } && !std::same_as<typename T::response_type, void>;
}

/**
 * A command response can be void (no response expected), or any type that can be constructed
 * from a vector of uint8_t.
 */
template<typename T>
concept CommandResponse = std::same_as<T, void> || requires {
    Frasy::Serialize(std::declval<T>());
    Frasy::Deserialize<T>(nullptr, nullptr);
};

template<typename T>
concept ValidCommandResponse =
  std::same_as<typename T::response_type, void> || CommandResponse<typename T::response_type>;
}    // namespace Frasy::Commands

#endif    // GUARD_NILAI_INTERFACES_COMMANDS_RESPONSE_H
