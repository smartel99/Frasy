/**
 * @file    type_manager.cpp
 * @author  Paul Thomas
 * @date    2023-02-14
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

#include <string_view>

#include "manager.h"
#include <stdexcept>

namespace Frasy::Type
{
static const char* s_tag               = "Type::Manager";
Manager            Manager::s_instance = Manager();

const char* Manager::InvalidIdException::what() const
{
    return "Invalid Type ID";
}

const char* Manager::TypeNotFoundException::what() const
{
    return "Type not found";
}

type_id_t Manager::AddStruct(const Struct& obj)
{
    while (s_instance.IsIdTaken(s_instance.m_count)) { ++s_instance.m_count; }
    s_instance.m_structs[s_instance.m_count] = obj;
    return s_instance.m_count;
}

type_id_t Manager::AddStruct(type_id_t id, const Struct& obj)
{
    if (s_instance.IsIdTaken(id)) { throw InvalidIdException(); }
    s_instance.m_structs[id] = obj;
    return id;
}

type_id_t Manager::AddEnum(const Enum& obj)
{
    while (s_instance.IsIdTaken(s_instance.m_count)) { ++s_instance.m_count; }
    s_instance.m_enums[s_instance.m_count] = obj;
    return s_instance.m_count;
}

type_id_t Manager::AddEnum(type_id_t id, const Enum& obj)
{
    if (s_instance.IsIdTaken(id)) { throw InvalidIdException(); }
    s_instance.m_enums[id] = obj;
    return id;
}

const Struct& Manager::GetStruct(type_id_t id)
{
    try
    {
        return s_instance.m_structs.at(id);
    }
    catch (std::out_of_range& e)
    {
        throw TypeNotFoundException();
    }
}

const Struct& Manager::GetStruct(const std::string& name)
{
    for (const auto& [id, obj] : s_instance.m_structs)
    {
        if (obj.Name == name) { return obj; }
    }
    throw TypeNotFoundException();
}

type_id_t Manager::GetStructId(const std::string& name)
{
    for (const auto& [id, obj] : s_instance.m_structs)
    {
        if (obj.Name == name) { return id; }
    }
    throw TypeNotFoundException();
}

std::vector<type_id_t> Manager::GetStructIds()
{
    std::vector<type_id_t> ids;
    ids.reserve(s_instance.m_structs.size());
    for (const auto& [id, obj] : s_instance.m_structs)
    {
        ids.push_back(id);
    }
    return ids;
}

const Enum& Manager::GetEnum(type_id_t id)
{
    try
    {
        return s_instance.m_enums.at(id);
    }
    catch (std::out_of_range& e)
    {
        throw TypeNotFoundException();
    }
}

const Enum& Manager::GetEnum(const std::string& name)
{
    for (const auto& [id, obj] : s_instance.m_enums)
    {
        if (obj.Name == name) { return obj; }
    }
    throw TypeNotFoundException();
}

type_id_t Manager::GetEnumId(const std::string& name)
{
    for (const auto& [id, obj] : s_instance.m_enums)
    {
        if (obj.Name == name) { return id; }
    }
    throw TypeNotFoundException();
}

std::vector<type_id_t> Manager::GetEnumIds()
{
    std::vector<type_id_t> ids;
    ids.reserve(s_instance.m_enums.size());
    for (const auto& [id, obj] : s_instance.m_enums) { ids.push_back(id); }
    return ids;
}

std::string Manager::GetTypeName(type_id_t id)
{
    if (s_instance.m_structs.contains(id))
    {
        return {s_instance.m_structs[id].Name.begin(), s_instance.m_structs[id].Name.end()};
    }
    if (s_instance.m_enums.contains(id))
    {
        return {s_instance.m_enums[id].Name.begin(), s_instance.m_enums[id].Name.end()};
    }
    static std::string error;
    error = "Unknown type: " + std::to_string(id);
    return error;
}

bool Manager::IsIdTaken(type_id_t id) { return m_structs.contains(id) || m_enums.contains(id); }

void Manager::AddFundamentals()
{
    for (const auto& [type, name] : std::vector<std::tuple<Fundamental::E, std::string>> {
           {Fundamental::E::Void, "Void"},
           {Fundamental::E::Bool, "Bool"},
           {Fundamental::E::Int8, "Int8"},
           {Fundamental::E::UInt8, "UInt8"},
           {Fundamental::E::Int16, "Int16"},
           {Fundamental::E::UInt16, "UInt16"},
           {Fundamental::E::Int32, "Int32"},
           {Fundamental::E::UInt32, "UInt32"},
           {Fundamental::E::Int64, "Int64"},
           {Fundamental::E::UInt64, "UInt64"},
           {Fundamental::E::Float, "Float"},
           {Fundamental::E::Double, "Double"},
           {Fundamental::E::String, "String"},
         })
    {
        AddStruct(static_cast<type_id_t>(type), Struct {.Name = name});
    }
    m_count = static_cast<std::size_t>(Fundamental::E::Size);
}


void Manager::ResetForTests() { s_instance = Manager(); }
}    // namespace Frasy::Type