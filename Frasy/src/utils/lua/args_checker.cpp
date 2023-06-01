/**
 * @file    args_checker.cpp
 * @author  Paul Thomas
 * @date    3/27/2023
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

#include "args_checker.h"

#include <format>
#include <stdexcept>

namespace Frasy::Lua
{
namespace
{
void CheckFundamental(
  const Frasy::Type::Manager& typeManager, const Frasy::Type::Struct::Field& field, const sol::object& object);
void CheckTable(
  const Frasy::Type::Manager& typeManager, const std::vector<Frasy::Type::Struct::Field>& fields, sol::table table);
void CheckContainer(
  const Frasy::Type::Manager&       typeManager,
  const Frasy::Type::Struct::Field& field,
  const std::vector<sol::object>&   objects);

void CheckFundamental(
  const Frasy::Type::Manager& typeManager, const Frasy::Type::Struct::Field& field, const sol::object& object)
{
    auto ot = object.get_type();

    switch (ot)
    {
        case sol::type::boolean:
            if (field.Type != static_cast<type_id_t>(Frasy::Type::Fundamental::E::Bool))
            {
                throw std::runtime_error(std::format(
                  "A bool cannot be assigned to field {} of type {}",
                  field.Name,
                  Frasy::Type::Fundamental::ToStr(static_cast<Type::Fundamental::E>(field.Type))));
            }
            break;
        case sol::type::string:
            if (field.Type != static_cast<type_id_t>(Frasy::Type::Fundamental::E::String))
            {
                throw std::runtime_error(std::format(
                  "A string cannot be assigned to field {} of type {}",
                  field.Name,
                  Frasy::Type::Fundamental::ToStr(static_cast<Type::Fundamental::E>(field.Type))));
            }
            break;
        case sol::type::number:
            if (
              field.Type < static_cast<type_id_t>(Frasy::Type::Fundamental::E::Int8) ||
              (field.Type > static_cast<type_id_t>(Frasy::Type::Fundamental::E::Double) &&
               !typeManager.IsEnum(field.Type)))
            {
                throw std::runtime_error(std::format(
                  "A number cannot be assigned to field {} of type {}",
                  field.Name,
                  Frasy::Type::Fundamental::ToStr(static_cast<Type::Fundamental::E>(field.Type))));
            }
            break;
        case sol::type::table:
        case sol::type::thread:
        case sol::type::function:
        case sol::type::userdata:
        case sol::type::lightuserdata:
        case sol::type::poly:
        case sol::type::none:
            throw std::runtime_error(
              std::format("Received type cannot be assigned to field {} (type id: {})", field.Name, field.Type));
        case sol::type::lua_nil:
            throw std::runtime_error(
              std::format("Cannot assign nil to field {} (type id: {})", field.Name, field.Type));
    }
}

void CheckFieldType(
  const Frasy::Type::Manager& typeManager, const Frasy::Type::Struct::Field& field, const sol::object& object)
{
    switch (object.get_type())
    {
        case sol::type::boolean:
        case sol::type::string:
        case sol::type::number: CheckFundamental(typeManager, field, object); break;
        case sol::type::table:
            if(typeManager.IsStruct(field.Type))
            {
                CheckTable(typeManager, typeManager.GetStruct(field.Type).Fields, object);
            }
            else if(field.Count != 1)
            {
                // Type is an array where all items should be the same.
                auto args = object.as<std::vector<sol::object>>();
                for(auto&& arg: args)
                {
                    CheckFieldType(typeManager, field, arg);
                }
            }

            break;
        case sol::type::thread:
        case sol::type::function:
        case sol::type::userdata:
        case sol::type::lightuserdata:
        case sol::type::poly:
        case sol::type::none:
            throw std::runtime_error(std::format(
              "Received type cannot be assigned to field {} (type id: {})", field.Name, field.Type));
        case sol::type::lua_nil:
            throw std::runtime_error(
              std::format("Cannot assign nil to field {} (type id: {})", field.Name, field.Type));
    }
}

void CheckTable(
  const Frasy::Type::Manager& typeManager, const std::vector<Frasy::Type::Struct::Field>& fields, sol::table table)
{
    std::size_t size = 0;
    for (const auto& [k, v] : table) { ++size; }
    if (size != fields.size())
    {
        throw std::runtime_error(
          std::format("Received a table with {} elements, however the type only has {} fields", size, fields.size()));
    }
    for (const auto& field : fields)
    {
        sol::object o  = table[field.Name];
        if (field.Count == Type::SINGLE)
        {
            CheckFieldType(typeManager, field, o);
        }
        else { CheckContainer(typeManager, field, o.as<std::vector<sol::object>>()); }
    }
}

void CheckContainer(
  const Frasy::Type::Manager&       typeManager,
  const Frasy::Type::Struct::Field& field,
  const std::vector<sol::object>&   objects)
{
    if (field.Count != Type::VECTOR && field.Count != objects.size())
    {
        throw std::logic_error(
          std::format("Expected an array of {} elements, only received {}", field.Count, objects.size()));
    }
    for (const auto& o : objects)
    {
        CheckFieldType(typeManager, field, o);
    }
}
}    // namespace
void CheckArgs(
  sol::state_view                                lua,
  const Frasy::Type::Manager&                    typeManager,
  const std::vector<Frasy::Type::Struct::Field>& fields,
  sol::variadic_args&                            args)
{
    if (args.size() != fields.size())
    {
        throw std::logic_error(std::format("Missing arguments! Expected {}, got {}", args.size(), fields.size()));
    }
    for (std::size_t i = 0; i < args.size(); ++i)
    {
        const auto& field = fields[i];
        sol::object arg   = args[i];

        if (field.Count == Type::SINGLE)
        {
            CheckFieldType(typeManager, field, arg);
        }
        else { CheckContainer(typeManager, field, args[i].as<std::vector<sol::object>>()); }
    }
}

}    // namespace Frasy::Lua
