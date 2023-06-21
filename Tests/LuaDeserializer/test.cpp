/**
 * @file    test.cpp
 * @author  Paul Thomas
 * @date    2023-02-28
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

#include "utils/lua/orchestrator/orchestrator.h"
#include "utils/lua/table_deserializer.h"
#include "utils/lua/table_serializer.h"

#include <gtest/gtest.h>
#include <iostream>

using Frasy::Actions::cmd_id_t;
using Frasy::Actions::Command;
using Frasy::Actions::Value;

using Frasy::Type::Enum;
using Frasy::Type::Fundamental;
using Frasy::Type::Struct;

static constexpr cmd_id_t command_two_ints       = 0;
static constexpr cmd_id_t command_simple_struct  = command_two_ints + 1;
static constexpr cmd_id_t command_complex_struct = command_simple_struct + 1;

Frasy::Type::Manager type_manager;

struct TwoInts
{
    int32_t a;
    int32_t b;
};

std::unordered_map<cmd_id_t, Command> commands;

type_id_t simple_struct_id  = 0;
type_id_t complex_struct_id = 0;
type_id_t enum_id           = 0;

Frasy::Type::Struct simple_struct_info;
Frasy::Type::Struct complex_struct_info;

Frasy::Type::Enum enum_info = Enum {
  .Name = "CustomEnum",
  .Fields =
    {
      Enum::Field {
        .Name  = "V1",
        .Value = 0,
      },
      Enum::Field {
        .Name  = "V2",
        .Value = 1,
      },
      Enum::Field {
        .Name  = "V3",
        .Value = 2,
      },
    },
};

enum class CustomEnum : uint32_t
{
    V1,
    V2,
    V3,
};

struct SimpleStruct
{
    uint16_t                   m_integer        = {};
    std::vector<uint16_t>      m_integer_vector = {};
    std::array<uint16_t, 3>    m_integer_array  = {};
    std::string                m_string         = {};
    std::vector<std::string>   m_string_vector  = {};
    std::array<std::string, 3> m_string_array   = {};
    CustomEnum                 m_enum           = {};
    std::vector<CustomEnum>    m_enum_vector    = {};
    std::array<CustomEnum, 3>  m_enum_array     = {};

    bool operator==(const SimpleStruct& other) const = default;
    bool operator==(const sol::table& other) const
    {
        return m_integer == other["m_integer"].get<decltype(m_integer)>() &&                         //
               other["m_integer_vector"].get<decltype(m_integer_vector)>() == m_integer_vector &&    //
               other["m_integer_array"].get<decltype(m_integer_array)>() == m_integer_array &&       //
               other["m_string"].get<decltype(m_string)>() == m_string &&                            //
               other["m_string_vector"].get<decltype(m_string_vector)>() == m_string_vector &&       //
               other["m_string_array"].get<decltype(m_string_array)>() == m_string_array &&          //
               other["m_enum"].get<decltype(m_enum)>() == m_enum &&                                  //
               other["m_enum_vector"].get<decltype(m_enum_vector)>() == m_enum_vector &&             //
               other["m_enum_array"].get<decltype(m_enum_array)>() == m_enum_array;
    }

    sol::table toTable(sol::state& lua) const
    {
        sol::table table          = lua.create_table();
        table["m_integer"]        = m_integer;
        table["m_integer_vector"] = sol::as_table(m_integer_vector);
        table["m_integer_array"]  = sol::as_table(m_integer_array);
        table["m_string"]         = m_string;
        table["m_string_vector"]  = sol::as_table(m_string_vector);
        table["m_string_array"]   = sol::as_table(m_string_array);
        table["m_enum"]           = m_enum;
        table["m_enum_vector"]    = sol::as_table(m_enum_vector);
        table["m_enum_array"]     = sol::as_table(m_enum_array);
        return table;
    }

    static SimpleStruct fromTable(const sol::table& table)
    {
        SimpleStruct object;
        object.m_integer        = table["m_integer"].get<decltype(m_integer)>();
        object.m_integer_vector = table["m_integer_vector"].get<decltype(m_integer_vector)>();
        object.m_integer_array  = table["m_integer_array"].get<decltype(m_integer_array)>();
        object.m_string         = table["m_string"].get<decltype(m_string)>();
        object.m_string_vector  = table["m_string_vector"].get<decltype(m_string_vector)>();
        object.m_string_array   = table["m_string_array"].get<decltype(m_string_array)>();
        object.m_enum           = table["m_enum"].get<decltype(m_enum)>();
        object.m_enum_vector    = table["m_enum_vector"].get<decltype(m_enum_vector)>();
        object.m_enum_array     = table["m_enum_array"].get<decltype(m_enum_array)>();
        return object;
    }
};

struct ComplexStruct
{
    SimpleStruct                m_simple_object        = {};
    std::vector<SimpleStruct>   m_simple_object_vector = {};
    std::array<SimpleStruct, 3> m_simple_object_array  = {};
    double                      m_control              = {};

    bool operator==(const ComplexStruct&) const = default;

    sol::table toTable(sol::state& lua) const
    {
        sol::table table                = lua.create_table();
        table["m_simple_object"]        = m_simple_object.toTable(lua);
        table["m_simple_object_vector"] = lua.create_table();
        std::for_each(m_simple_object_vector.begin(),
                      m_simple_object_vector.end(),
                      [&](const SimpleStruct& object)
                      { table.get<sol::table>("m_simple_object_vector").add(object.toTable(lua)); });
        table["m_simple_object_array"] = lua.create_table();
        std::for_each(m_simple_object_array.begin(),
                      m_simple_object_array.end(),
                      [&](const SimpleStruct& object)
                      { table.get<sol::table>("m_simple_object_array").add(object.toTable(lua)); });
        table["m_control"] = m_control;
        return table;
    }

    static ComplexStruct fromTable(const sol::table& table)
    {
        ComplexStruct object;
        object.m_simple_object = SimpleStruct::fromTable(table["m_simple_object"]);
        object.m_simple_object_vector.resize(3);
        object.m_simple_object_vector[0] = SimpleStruct::fromTable(table["m_simple_object_vector"][1]);
        object.m_simple_object_vector[1] = SimpleStruct::fromTable(table["m_simple_object_vector"][2]);
        object.m_simple_object_vector[2] = SimpleStruct::fromTable(table["m_simple_object_vector"][3]);
        object.m_simple_object_array[0]  = SimpleStruct::fromTable(table["m_simple_object_array"][1]);
        object.m_simple_object_array[1]  = SimpleStruct::fromTable(table["m_simple_object_array"][2]);
        object.m_simple_object_array[2]  = SimpleStruct::fromTable(table["m_simple_object_array"][3]);
        object.m_control                 = table["m_control"];
        return object;
    }
};

TEST(LuaInterpreter, SimpleToTable)
{
    sol::state lua;

    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::table);

    SimpleStruct simple = {
      .m_integer        = 1,
      .m_integer_vector = {2, 3, 4},
      .m_integer_array  = {5, 6, 7},
      .m_string         = "str1",
      .m_string_vector  = {"str2", "str3", "str4"},
      .m_string_array   = {"str5", "str6", "str7"},
      .m_enum           = CustomEnum::V2,
      .m_enum_vector    = {CustomEnum::V3, CustomEnum::V2, CustomEnum::V1},
      .m_enum_array     = {CustomEnum::V3, CustomEnum::V1, CustomEnum::V2},
    };
    std::vector<uint8_t> expected = Frasy::Serialize(simple);

    lua["simple_table"]           = simple.toTable(lua);
    sol::table simple_table_order = lua.create_table("simple_table_order");
    simple_table_order.add("m_integer",
                           "m_integer_vector",
                           "m_integer_array",
                           "m_string",
                           "m_string_vector",
                           "m_string_array",
                           "m_enum",
                           "m_enum_vector",
                           "m_enum_array");

    lua.require_file("Utils", "lua/utils.lua");

    lua.set_function("ToSimple",
                     [&](sol::variadic_args args)
                     {
                         sol::table table = lua.create_table();
                         Frasy::Lua::ArgsToTable(
                           table, type_manager, type_manager.GetStruct(simple_struct_id).Fields, args);
                         lua["simple_table_imported"] = table;
                         lua.script("are_equals = Utils.Equals(simple_table, simple_table_imported)");
                         ASSERT_TRUE(lua.get<bool>("are_equals"));
                     });

    lua.script("ToSimple(table.unpack(Utils.Table.ToList(simple_table, simple_table_order)))");
}

TEST(LuaInterpreter, ComplexToTable)
{
    sol::state lua;

    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::table);

    SimpleStruct simple = {
      .m_integer        = 1,
      .m_integer_vector = {2, 3, 4},
      .m_integer_array  = {5, 6, 7},
      .m_string         = "str1",
      .m_string_vector  = {"str2", "str3", "str4"},
      .m_string_array   = {"str5", "str6", "str7"},
      .m_enum           = CustomEnum::V2,
      .m_enum_vector    = {CustomEnum::V3, CustomEnum::V2, CustomEnum::V1},
      .m_enum_array     = {CustomEnum::V3, CustomEnum::V1, CustomEnum::V2},
    };
    ComplexStruct complex = {
      .m_simple_object        = simple,
      .m_simple_object_vector = {simple, simple, simple},
      .m_simple_object_array  = {simple, simple, simple},
      .m_control              = 3.14,
    };

    lua["complex_table"]           = complex.toTable(lua);
    sol::table complex_table_order = lua.create_table("complex_table_order");
    complex_table_order.add("m_simple_object", "m_simple_object_vector", "m_simple_object_array", "m_control");

    lua.require_file("Utils", "lua/utils.lua");

    lua.set_function("ToComplex",
                     [&](sol::variadic_args args)
                     {
                         sol::table table = lua.create_table();
                         Frasy::Lua::ArgsToTable(
                           table, type_manager, type_manager.GetStruct(complex_struct_id).Fields, args);
                         lua["complex_table_imported"] = table;
                         lua.script("are_equals = Utils.Equals(complex_table, complex_table_imported)");
                         ASSERT_TRUE(lua.get<bool>("are_equals"));
                     });

    lua.script("ToComplex(table.unpack(Utils.Table.ToList(complex_table, complex_table_order)))");
}

TEST(LuaInterpreter, SerializeSimple)
{
    sol::state lua;

    lua.open_libraries(sol::lib::base);

    SimpleStruct simple = {
      .m_integer        = 1,
      .m_integer_vector = {2, 3, 4},
      .m_integer_array  = {5, 6, 7},
      .m_string         = "str1",
      .m_string_vector  = {"str2", "str3", "str4"},
      .m_string_array   = {"str5", "str6", "str7"},
      .m_enum           = CustomEnum::V2,
      .m_enum_vector    = {CustomEnum::V3, CustomEnum::V2, CustomEnum::V1},
      .m_enum_array     = {CustomEnum::V3, CustomEnum::V1, CustomEnum::V2},
    };
    std::vector<uint8_t> expected = Frasy::Serialize(simple);

    lua["simple_table"] = simple.toTable(lua);

    lua.set_function("SerializeSimple",
                     [&](sol::table table)
                     {
                         std::vector<uint8_t> output;
                         Frasy::Lua::ParseTable(
                           table, type_manager, type_manager.GetStruct(simple_struct_id).Fields, output);
                         ASSERT_EQ(expected, output);
                     });

    lua.script("SerializeSimple(simple_table)");
}

TEST(LuaInterpreter, SerializeComplex)
{
    sol::state lua;

    lua.open_libraries(sol::lib::base);

    SimpleStruct simple = {
      .m_integer        = 1,
      .m_integer_vector = {2, 3, 4},
      .m_integer_array  = {5, 6, 7},
      .m_string         = "str1",
      .m_string_vector  = {"str2", "str3", "str4"},
      .m_string_array   = {"str5", "str6", "str7"},
      .m_enum           = CustomEnum::V2,
      .m_enum_vector    = {CustomEnum::V3, CustomEnum::V2, CustomEnum::V1},
      .m_enum_array     = {CustomEnum::V3, CustomEnum::V1, CustomEnum::V2},
    };
    ComplexStruct complex = {
      .m_simple_object        = simple,
      .m_simple_object_vector = {simple, simple, simple},
      .m_simple_object_array  = {simple, simple, simple},
      .m_control              = 3.14,
    };
    std::vector<uint8_t> expected = Frasy::Serialize(complex);

    lua["complex_table"] = complex.toTable(lua);

    lua.set_function("SerializeComplex",
                     [&](sol::table table)
                     {
                         std::vector<uint8_t> output;
                         Frasy::Lua::ParseTable(
                           table, type_manager, type_manager.GetStruct(complex_struct_id).Fields, output);
                         ASSERT_EQ(expected, output);
                     });

    lua.script("SerializeComplex(complex_table)");
}

TEST(LuaInterpreter, DeserializeSimple)
{
    sol::state   lua;
    SimpleStruct obj = {
      .m_integer        = 1,
      .m_integer_vector = {2, 3, 4},
      .m_integer_array  = {5, 6, 7},
      .m_string         = "str1",
      .m_string_vector  = {"str2", "str3", "str4"},
      .m_string_array   = {"str5", "str6", "str7"},
      .m_enum           = CustomEnum::V2,
      .m_enum_vector    = {CustomEnum::V3, CustomEnum::V2, CustomEnum::V1},
      .m_enum_array     = {CustomEnum::V3, CustomEnum::V1, CustomEnum::V2},
    };

    auto data  = Frasy::Serialize(obj);
    auto table = Frasy::Lua::Deserialize(
      lua, type_manager.GetStruct(simple_struct_id).Fields, type_manager.GetStructs(), type_manager.GetEnums(), data);

    SimpleStruct loaded = SimpleStruct::fromTable(table);
    ASSERT_EQ(loaded, obj);
}

TEST(LuaInterpreter, DeserializeComplex)
{
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    SimpleStruct sobj = {
      .m_integer        = 1,
      .m_integer_vector = {2, 3, 4},
      .m_integer_array  = {5, 6, 7},
      .m_string         = "str1",
      .m_string_vector  = {"str2", "str3", "str4"},
      .m_string_array   = {"str5", "str6", "str7"},
      .m_enum           = CustomEnum::V2,
      .m_enum_vector    = {CustomEnum::V3, CustomEnum::V2, CustomEnum::V1},
      .m_enum_array     = {CustomEnum::V3, CustomEnum::V1, CustomEnum::V2},
    };

    ComplexStruct obj = {sobj, {sobj, sobj, sobj}, {sobj, sobj, sobj}, 4.32};

    auto data  = Frasy::Serialize(obj);
    auto table = Frasy::Lua::Deserialize(
      lua, type_manager.GetStruct(complex_struct_id).Fields, type_manager.GetStructs(), type_manager.GetEnums(), data);
    lua.set("custom_complex_obj", table);

    ComplexStruct loaded = ComplexStruct::fromTable(table);
    ASSERT_EQ(loaded, obj);
}

int main(int argc, char** argv)
{
    enum_id            = type_manager.AddEnum(enum_info);
    simple_struct_info = Struct {
      .Name = "SimpleStruct",
      .Fields =
        {
          Struct::Field {
            .Name = "m_integer",
            .Type = static_cast<type_id_t>(Fundamental::E::UInt16),
          },
          Struct::Field {
            .Name  = "m_integer_vector",
            .Type  = static_cast<type_id_t>(Fundamental::E::UInt16),
            .Count = Frasy::Type::VECTOR,
          },
          Struct::Field {
            .Name  = "m_integer_array",
            .Type  = static_cast<type_id_t>(Fundamental::E::UInt16),
            .Count = Frasy::Type::ARRAY(3),
          },
          Struct::Field {
            .Name = "m_string",
            .Type = static_cast<type_id_t>(Fundamental::E::String),
          },
          Struct::Field {
            .Name  = "m_string_vector",
            .Type  = static_cast<type_id_t>(Fundamental::E::String),
            .Count = Frasy::Type::VECTOR,
          },
          Struct::Field {
            .Name  = "m_string_array",
            .Type  = static_cast<type_id_t>(Fundamental::E::String),
            .Count = Frasy::Type::ARRAY(3),
          },
          Struct::Field {
            .Name = "m_enum",
            .Type = enum_id,
          },
          Struct::Field {
            .Name  = "m_enum_vector",
            .Type  = enum_id,
            .Count = Frasy::Type::VECTOR,
          },
          Struct::Field {
            .Name  = "m_enum_array",
            .Type  = enum_id,
            .Count = Frasy::Type::ARRAY(3),
          },
        },
    };
    simple_struct_id = type_manager.AddStruct(simple_struct_info);

    complex_struct_info = Struct {
      .Name = "ComplexStruct",
      .Fields =
        {
          Struct::Field {
            .Name = "m_simple_object",
            .Type = simple_struct_id,
          },
          Struct::Field {
            .Name  = "m_simple_object_vector",
            .Type  = simple_struct_id,
            .Count = Frasy::Type::VECTOR,
          },
          Struct::Field {
            .Name  = "m_simple_object_array",
            .Type  = simple_struct_id,
            .Count = Frasy::Type::ARRAY(3),
          },
          Struct::Field {
            .Name = "m_control",
            .Type = static_cast<type_id_t>(Fundamental::E::Double),
          },
        },
    };

    complex_struct_id = type_manager.AddStruct(complex_struct_info);

    commands = {
      {
        command_two_ints,
        Command {
          .Id = command_two_ints,
          .Parameters =
            {
              Value {
                .Pos  = 0,
                .Name = "arg1",
                .Type = static_cast<type_id_t>(Fundamental::E::Int32),
              },
              Value {
                .Pos  = 1,
                .Name = "arg2",
                .Type = static_cast<type_id_t>(Fundamental::E::Int32),
              },
            },
          .Returned =
            {
              Value {
                .Pos  = 0,
                .Name = "arg1",
                .Type = static_cast<type_id_t>(Fundamental::E::Int32),
              },
              Value {
                .Pos  = 1,
                .Name = "arg2",
                .Type = static_cast<type_id_t>(Fundamental::E::Int32),
              },
            },
        },
      },
      {
        command_simple_struct,
        Command {
          .Id = command_simple_struct,
          .Parameters =
            {
              Value {
                .Pos  = 0,
                .Name = "object",
                .Type = simple_struct_id,
              },
            },
          .Returned =
            {
              Value {
                .Pos  = 0,
                .Name = "object",
                .Type = simple_struct_id,
              },
            },
        },
      },
      {
        command_complex_struct,
        Command {
          .Id = command_complex_struct,
          .Parameters =
            {
              Value {
                .Pos  = 0,
                .Name = "object",
                .Type = complex_struct_id,
              },
            },
          .Returned =
            {
              Value {
                .Pos  = 0,
                .Name = "object",
                .Type = complex_struct_id,
              },
            },
        },
      },
    };

    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}