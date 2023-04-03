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

namespace Frasy::Lua
{

void CheckFundamental(const Frasy::Type::Manager&       typeManager,
                      const Frasy::Type::Struct::Field& field,
                      sol::object                       object);

void CheckTable(const Frasy::Type::Manager&                    typeManager,
                const std::vector<Frasy::Type::Struct::Field>& fields,
                sol::table                                     table);

void CheckContainer(const Frasy::Type::Manager& typeManager,
                    Frasy::Type::Struct::Field  field,
                    std::vector<sol::object>    objects);

void CheckArgs(sol::state&                                    lua,
               const Frasy::Type::Manager&                    typeManager,
               const std::vector<Frasy::Type::Struct::Field>& fields,
               sol::variadic_args&                            args)
{
    if (args.size() != fields.size()) throw std::exception();
    for (std::size_t i = 0; i < args.size(); ++i)
    {
        const auto& field = fields[i];
        sol::object arg   = args[i];

        if (field.Count == Type::SINGLE)
        {
            auto at = arg.get_type();
            switch (at)
            {
                case sol::type::boolean:
                case sol::type::string:
                case sol::type::number: CheckFundamental(typeManager, field, arg); break;
                case sol::type::table:
                    if (!typeManager.IsStruct(field.Type)) throw std::exception();
                    CheckTable(typeManager, typeManager.GetStruct(field.Type).Fields, arg);
                    break;
                case sol::type::thread:
                case sol::type::function:
                case sol::type::userdata:
                case sol::type::lightuserdata:
                case sol::type::poly:
                case sol::type::none:
                case sol::type::lua_nil: throw std::exception();
            }
        }
        else { CheckContainer(typeManager, field, args[i].as<std::vector<sol::object>>()); }
    }
}

void CheckFundamental(const Frasy::Type::Manager&       typeManager,
                      const Frasy::Type::Struct::Field& field,
                      sol::object                       object)
{
    auto ot = object.get_type();
    switch (ot)
    {
        case sol::type::boolean:
            if (field.Type != static_cast<type_id_t>(Frasy::Type::Fundamental::E::Bool)) throw std::exception();
            break;
        case sol::type::string:
            if (field.Type != static_cast<type_id_t>(Frasy::Type::Fundamental::E::String)) throw std::exception();
            break;
        case sol::type::number:
            if (field.Type < static_cast<type_id_t>(Frasy::Type::Fundamental::E::Int8) ||
                (field.Type > static_cast<type_id_t>(Frasy::Type::Fundamental::E::Double) &&
                 !typeManager.IsEnum(field.Type)))
                throw std::exception();
            break;
        case sol::type::table:
        case sol::type::thread:
        case sol::type::function:
        case sol::type::userdata:
        case sol::type::lightuserdata:
        case sol::type::poly:
        case sol::type::none:
        case sol::type::lua_nil: throw std::exception();
    }
}

void CheckTable(const Frasy::Type::Manager&                    typeManager,
                const std::vector<Frasy::Type::Struct::Field>& fields,
                sol::table                                     table)
{
    std::size_t size = 0;
    for (const auto& [k, v] : table) { ++size; }
    if (size != fields.size()) throw std::exception();
    for (const auto& field : fields)
    {
        sol::object o  = table[field.Name];
        auto        ot = o.get_type();
        if (field.Count == Type::SINGLE)
        {
            switch (ot)
            {
                case sol::type::boolean:
                case sol::type::string:
                case sol::type::number: CheckFundamental(typeManager, field, o); break;
                case sol::type::table:
                    if (!typeManager.IsStruct(field.Type)) throw std::exception();
                    CheckTable(typeManager, typeManager.GetStruct(field.Type).Fields, o);
                    break;
                case sol::type::thread:
                case sol::type::function:
                case sol::type::userdata:
                case sol::type::lightuserdata:
                case sol::type::poly:
                case sol::type::none:
                case sol::type::lua_nil: throw std::exception();
            }
        }
        else { CheckContainer(typeManager, field, o.as<std::vector<sol::object>>()); }
    }
}

void CheckContainer(const Frasy::Type::Manager& typeManager,
                    Frasy::Type::Struct::Field  field,
                    std::vector<sol::object>    objects)
{
    if (field.Count != Type::VECTOR && field.Count != objects.size()) throw std::exception();
    for (const auto& o : objects)
    {
        auto ot = o.get_type();
        switch (ot)
        {
            case sol::type::boolean:
            case sol::type::string:
            case sol::type::number: CheckFundamental(typeManager, field, o); break;
            case sol::type::table:
                if (!typeManager.IsStruct(field.Type)) throw std::exception();
                CheckTable(typeManager, typeManager.GetStruct(field.Type).Fields, o);
                break;
            case sol::type::thread:
            case sol::type::function:
            case sol::type::userdata:
            case sol::type::lightuserdata:
            case sol::type::poly:
            case sol::type::none:
            case sol::type::lua_nil: throw std::exception();
        }
    }
}
}    // namespace Frasy::Lua
