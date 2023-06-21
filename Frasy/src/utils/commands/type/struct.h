/**
 * @file    struct.h
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
#ifndef INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_STRUCT_H
#define INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_STRUCT_H

#include "count.h"
#include "fundamental.h"
#include "type_id_t.h"
#include "utils/misc/vector_to_string.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Frasy::Type
{
struct Struct
{
    struct Field
    {
        struct Manager;
        std::string Name;
        type_id_t   Type        = 0;
        uint16_t    Count       = 1;
        std::string Description = {};

        Field& SetName(const std::string& name)
        {
            Name = name;
            return *this;
        }

        template<typename T>
            requires std::same_as<T, type_id_t> ||
                     (std::is_enum_v<T> && std::same_as<std::underlying_type_t<T>, type_id_t>)
        Field& SetType(T id)
        {
            Type = static_cast<type_id_t>(id);
            return *this;
        }

        Field& SetCount(uint16_t count)
        {
            Count = count;
            return *this;
        }

        Field& SetDescription(const std::string& description)
        {
            Description = description;
            return *this;
        }

        bool operator==(const Field& other) const
        {
            return Name == other.Name &&    //
                   Type == other.Type &&    //
                   Count == other.Count;
        }

        explicit operator std::string() const
        {
            return "{" + std::string(Name) + ", " + std::to_string(Type) + ", " + std::to_string(Count) + ", " +
                   std::string(Description) + "}";
        }
    };

    struct Manager;
    std::string        Name;
    std::vector<Field> Fields      = {};
    std::string        Description = {};

    Struct& SetName(const std::string& name)
    {
        Name = name;
        return *this;
    }

    Struct& AddField(const Field& field)
    {
        Fields.push_back(field);
        return *this;
    }

    Struct& SetFields(std::vector<Field> fields)
    {
        Fields = std::move(fields);
        return *this;
    }

    Struct& SetDescription(const std::string& description)
    {
        Description = description;
        return *this;
    }

    bool operator==(const Struct& other) const
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

struct Struct::Field::Manager
{
    static constexpr std::string_view name        = "Struct::Field";
    static constexpr std::string_view description = "Representation of a field in a custom struct";
    static type_id_t                  id;

    // Fields
    struct Name
    {
        static constexpr std::string_view name        = "Name";
        static constexpr std::string_view description = "Name of the field";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
    };
    struct Type
    {
        static constexpr std::string_view name        = "Type";
        static constexpr std::string_view description = "Tell the field type";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::UInt16);
    };
    struct Count
    {
        static constexpr std::string_view name = "Count";
        static constexpr std::string_view description =
          "Tell how many objects are stored in this field. 0 means vector";
        static constexpr type_id_t type = static_cast<type_id_t>(Fundamental::E::UInt16);
    };
    struct Description
    {
        static constexpr std::string_view name        = "Description";
        static constexpr std::string_view description = "Provide information about the field";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
    };
};

struct Struct::Manager
{
    static constexpr std::string_view name        = "Struct";
    static constexpr std::string_view description = "Representation of a custom struct";
    static type_id_t                  id;

    // Fields
    struct Name
    {
        static constexpr std::string_view name        = "Name";
        static constexpr std::string_view description = "Name of the struct";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
    };
    struct Fields
    {
        static constexpr std::string_view name        = "Fields";
        static constexpr std::string_view description = "Fields populating this struct";
        static type_id_t                  type() { return Field::Manager::id; }
        static constexpr uint16_t         count = Type::VECTOR;
    };
    struct Description
    {
        static constexpr std::string_view name        = "Description";
        static constexpr std::string_view description = "Provide information about the struct";
        static constexpr type_id_t        type        = static_cast<type_id_t>(Fundamental::E::String);
    };
};

}    // namespace Frasy::Type

#endif    // INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_STRUCT_H
