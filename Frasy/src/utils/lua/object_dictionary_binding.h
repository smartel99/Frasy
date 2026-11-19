/**
 * @file    object_dictionary_binding.h
 * @author  Paul Thomas
 * @date    2026-11-19
 * @brief   Gives the object dictionaries parsed by C++ to Lua.
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this
 * program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef FRASY_UTILS_LUA_OBJECT_DICTIONARY_BINDING_H
#define FRASY_UTILS_LUA_OBJECT_DICTIONARY_BINDING_H

#include <sol/sol.hpp>

namespace Frasy::Lua {

/**
 * @brief Registers the functions that give the content of an EDS or XDD file to Lua.
 *
 * - `__loadObjectDictionary(path)`: the dictionary of a file. The file is parsed once, the result is shared with the
 *   other users of the dictionary (see ObjectDictionaryRegistry).
 * - `__parseObjectDictionary(text)`: the dictionary described by the content of a file.
 *
 * Both return the sections of the dictionary, with the layout of an EDS: `XXXX` for an object and `XXXXsubY` for one of
 * its sub-indexes, whose fields are named like the keys of an EDS (`ParameterName`, `ObjectType`, `DataType`, ...).
 * Lua builds its own table from them (see `lua/core/can_open/object_dictionary.lua`). They raise an error when the file
 * cannot be used.
 */
void registerObjectDictionary(sol::state_view lua);

}    // namespace Frasy::Lua

#endif    // FRASY_UTILS_LUA_OBJECT_DICTIONARY_BINDING_H
