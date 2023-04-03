/**
 * @file    value.h
 * @author  Samuel Martel
 * @date    2023-01-05
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

#ifndef FRASY_INTERFACES_COMMANDS_DESCRIPTION_VALUE_H
#define FRASY_INTERFACES_COMMANDS_DESCRIPTION_VALUE_H

#include "../../communication/serial/types.h"
#include "../type/count.h"
#include "../type/fundamental.h"
#include "../type/manager/manager.h"

#include <string>
#include <string_view>
#include <vector>

namespace Frasy::Actions
{
/**
 * Structure describing a command's value.
 * This value can either be part of the argument list taken by the command,
 * or it can be part of the response given by that command.
 */
struct Value
{
    uint16_t    Pos = 0;                 //!< Position of the value.
    std::string Name;                    //!< Name associated to the value.
    std::string Help;                    //!< Help message associated with the string.
    std::string Alias = "";              //!< Acceptable alias. // NOLINT(readability-redundant-string-init)
    type_id_t   Type  = 0;               //!< Type of the value.
    uint16_t    Count = Type::SINGLE;    //!< 1 if single value, 0 if vector, >1 if array
    std::string Min   = {};              //!< Minimal acceptable value for the argument.
    std::string Max   = {};              //!< Maximal acceptable value for the argument.

    static Value Make(const std::string& name, type_id_t type, uint16_t pos, const std::string& help)
    {
        return {.Pos = pos, .Name = name, .Help = help, .Alias = "", .Type = type};
    }

    static Value Make(const std::string& name, Type::Fundamental::E type, uint16_t pos, const std::string& help)
    {
        return {.Pos = pos, .Name = name, .Help = help, .Alias = "", .Type = static_cast<type_id_t>(type)};
    }

    Value& SetPos(uint16_t pos)
    {
        Pos = pos;
        return *this;
    }

    Value& SetName(const std::string& name)
    {
        Name = name;
        return *this;
    }

    Value& SetHelp(const std::string& help)
    {
        Help = help;
        return *this;
    }

    Value& SetAlias(const std::string& alias)
    {
        Alias = alias;
        return *this;
    }

    Value& SetType(type_id_t type)
    {
        Type = type;
        return *this;
    }

    Value& SetType(Frasy::Type::Fundamental::E type)
    {
        Type = static_cast<type_id_t>(type);
        return *this;
    }

    Value& SetCount(uint16_t count)
    {
        Count = count;
        return *this;
    }

    Value& SetMin(const std::string& min)
    {
        Min = min;
        return *this;
    }

    Value& SetMax(const std::string& max)
    {
        Max = max;
        return *this;
    }

    struct Manager;

    std::string ToString(const Frasy::Type::Manager& manager) const
    {
        return "{" + std::to_string(Pos) + ", " + std::string(Name) + ", " + std::string(Help) + ", " +
               std::string(Alias) + ", " + std::string(manager.GetTypeName(Type)) + ", " +
               std::to_string(Count) + ", " + std::string(Min) + ", " + std::string(Max) + "}";
    }

    explicit operator std::string() const {
        return "{" + std::to_string(Pos) + ", " + std::string(Name) + ", " + std::string(Help) + ", " +
               std::string(Alias) + ", " + "???" + ", " +
               std::to_string(Count) + ", " + std::string(Min) + ", " + std::string(Max) + "}";
    }
};

using Type::Fundamental;
struct Value::Manager
{

    static Communication::cmd_id_t    id;
    static constexpr std::string_view name        = "Value";
    static constexpr std::string_view description = "Define a value passed by or returned from a command";

    // fields
    struct Pos
    {
        static constexpr std::string_view name = "Pos";
        static constexpr std::string_view description =
          "Position of the value as a parameter for a command. Unused for returns";
        static constexpr type_id_t type = static_cast<type_id_t>(Fundamental::E::UInt16);
    };
    struct Name
    {
        static constexpr std::string_view name = "Name";
        static constexpr std::string_view description =
          "Name of the value. Can be used to define a parameters when invoking a command";
        static constexpr type_id_t type = static_cast<type_id_t>(Fundamental::E::String);
    };
    struct Help
    {
        static constexpr std::string_view name        = "Help";
        static constexpr std::string_view description = "Explain what this field represent";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
    };
    struct Alias
    {
        static constexpr std::string_view name = "Alias";
        static constexpr std::string_view description =
          "Alternative name for supplying a parameter when invoking a command. Optional";
        static constexpr type_id_t type = static_cast<type_id_t>(Fundamental::E::String);
    };
    struct Type
    {
        static constexpr std::string_view name        = "Type";
        static constexpr std::string_view description = "Tell the type of the value";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::UInt16);
    };
    struct Count
    {
        static constexpr std::string_view name = "Count";
        static constexpr std::string_view description =
          "Tell whether this value is an array or a single value. 1 means single value, >1 means it's an array, 0 "
          "means it's a vector.";
        static constexpr type_id_t type = static_cast<type_id_t>(Fundamental::E::UInt16);
    };
    struct Min
    {
        static constexpr std::string_view name = "Min";
        static constexpr std::string_view description =
          "Tell what is the minimal value accepted for that value. Only for numeric types and for command arguments.";
        static constexpr type_id_t type = static_cast<type_id_t>(Fundamental::E::String);
    };
    struct Max
    {
        static constexpr std::string_view name = "Max";
        static constexpr std::string_view description =
          "Tell what is the maximal value accepted for that value. Only for numeric types and for command arguments.";
        static constexpr type_id_t type = static_cast<type_id_t>(Fundamental::E::String);
    };
};

}    // namespace Frasy::Actions

#endif    // FRASY_INTERFACES_COMMANDS_DESCRIPTION_VALUE_H
