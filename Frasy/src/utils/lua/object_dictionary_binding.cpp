/**
 * @file    object_dictionary_binding.cpp
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
#include "object_dictionary_binding.h"

#include "utils/communication/can_open/object_dictionary.h"
#include "utils/communication/can_open/object_dictionary_registry.h"

#include <format>
#include <string>
#include <utility>

namespace Frasy::Lua {

namespace {
using CanOpen::OdEntry;
using CanOpen::OdObject;

void setEntryFields(sol::table& fields, const OdEntry& entry, const std::string& name)
{
    fields["ParameterName"] = name;
    fields["ObjectType"]    = std::to_underlying(entry.objectType);
    // 0 when the file does not say: Lua only needs a number to decide how to read the default value.
    fields["DataType"]   = entry.dataType;
    fields["AccessType"] = std::string {CanOpen::toString(entry.access)};
    fields["PDOMapping"] = entry.isPdoMappable ? 1 : 0;
    if (!entry.defaultValue.empty()) { fields["DefaultValue"] = entry.defaultValue; }
    if (!entry.lowLimit.empty()) { fields["LowLimit"] = entry.lowLimit; }
    if (!entry.highLimit.empty()) { fields["HighLimit"] = entry.highLimit; }
    if (entry.stringLengthMin.has_value()) { fields["StringLengthMin"] = *entry.stringLengthMin; }
}

sol::table toSections(sol::state_view lua, const CanOpen::ObjectDictionary& dictionary)
{
    auto sections = lua.create_table();
    for (const OdObject& object : dictionary.getObjects()) {
        const auto key    = std::format("{:04X}", object.index);
        auto       fields = lua.create_table();
        if (object.hasSubIndexes()) {
            fields["ParameterName"] = object.name;
            fields["ObjectType"]    = std::to_underlying(object.type);
            fields["SubNumber"]     = object.subNumber;
            for (const OdEntry& entry : object.entries) {
                auto subFields = lua.create_table();
                setEntryFields(subFields, entry, entry.name);
                sections[std::format("{}sub{:X}", key, entry.subIndex)] = subFields;
            }
        }
        else if (!object.entries.empty()) {
            setEntryFields(fields, object.entries.front(), object.name);
            fields["ObjectType"] = std::to_underlying(object.type);
        }
        sections[key] = fields;
    }
    return sections;
}
}    // namespace

void registerObjectDictionary(sol::state_view lua)
{
    lua.set_function("__loadObjectDictionary", [](sol::this_state state, const std::string& path) {
        auto result = CanOpen::ObjectDictionaryRegistry::get().load(path);
        if (!result.has_value()) { throw sol::error(result.error()); }
        return toSections(state, **result);
    });

    lua.set_function("__parseObjectDictionary", [](sol::this_state state, const std::string& text) {
        auto result = CanOpen::ObjectDictionary::parse(text);
        if (!result.has_value()) { throw sol::error(result.error()); }
        return toSections(state, *result);
    });
}

}    // namespace Frasy::Lua
