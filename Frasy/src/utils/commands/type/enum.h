/**
 * @file    enum.h
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
#ifndef INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_ENUM_H
#define INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_ENUM_H

#include "count.h"
#include "fundamental.h"
#include "type_id_t.h"
#include "utils/misc/vector_to_string.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Frasy::Type
{
struct Enum
{
    struct Field
    {
        std::string Name;
        uint32_t    Value       = 0;
        std::string Description = {};

        bool operator==(const Field& other) const
        {
            return Name == other.Name &&    //
                   Value == other.Value;
        }

        explicit operator std::string() const
        {
            return "{" + std::string(Name) +         //
                   ", " + std::to_string(Value) +    //
                   ", " + std::string(Description) + "}";
        }

        struct Manager;
    };

    std::string        Name;
    std::vector<Field> Fields      = {};
    std::string        Description = {};

    struct Manager;

    bool operator==(const Enum& other) const
    {
        if (Name != other.Name) return false;
        if (Fields.size() != other.Fields.size()) return false;
        for (std::size_t i = 0; i < Fields.size(); ++i)
        {
            if (Fields[i] != other.Fields[i]) return false;
        }
        return true;
    }

    explicit operator std::string() const
    {
        return "{" + std::string(Name) + ", " + VectorToString(Fields) + ", " + std::string(Description) + "}";
    }
};

struct Enum::Field::Manager
{
    static constexpr std::string_view name        = "Enum::Field";
    static constexpr std::string_view description = "Field of the enum";
    static type_id_t                  id;

    // Fields
    struct Name
    {
        static constexpr std::string_view name        = "Name";
        static constexpr std::string_view description = "Name of the field";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
    };
    struct Value
    {
        static constexpr std::string_view name        = "Value";
        static constexpr std::string_view description = "Numeric value of the field";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::UInt32);
    };
    struct Description
    {
        static constexpr std::string_view name        = "Description";
        static constexpr std::string_view description = "Provide information about the purpose of the field";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
    };
};

struct Enum::Manager
{
    static constexpr std::string_view name        = "Enum";
    static constexpr std::string_view description = "Definition of an enum";
    static type_id_t                  id;

    // Fields
    struct Name
    {
        static constexpr std::string_view name        = "Name";
        static constexpr std::string_view description = "Name of the enum";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
    };
    struct Fields
    {
        static constexpr std::string_view name        = "Fields";
        static constexpr std::string_view description = "Fields that populate this enum";
        static type_id_t                  type() { return Enum::Field::Manager::id; };
        static constexpr uint16_t         count = VECTOR;
    };
    struct Description
    {
        static constexpr std::string_view name        = "Description";
        static constexpr std::string_view description = "Provide information about the purpose of the enum";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
    };
};

}    // namespace Frasy::Type

#endif    // INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_ENUM_H
