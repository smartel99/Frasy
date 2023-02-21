/**
 * @file    type_manager.h
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
#ifndef INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_MANAGER_H
#define INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_MANAGER_H

#include "../basic_info.h"
#include "../enum.h"
#include "../fundamental.h"
#include "../struct.h"

#include <string>
#include <unordered_map>
#include <vector>


namespace Frasy::Type
{
class Manager
{
private:
    static Manager                        s_instance;
    type_id_t                             m_count   = 0;
    std::unordered_map<type_id_t, Struct> m_structs = {};
    std::unordered_map<type_id_t, Enum>   m_enums   = {};

    Manager()
    {
        m_structs.clear();
        m_enums.clear();
        AddFundamentals();
        AddBuiltInEnums();
        AddBuiltInStructs();
    }

public:
    class InvalidIdException : public std::exception
    {
    public:
        [[nodiscard]] const char* what() const final;
    };

    class TypeNotFoundException : public std::exception
    {
    public:
        [[nodiscard]] const char* what() const final;
    };

public:
    /// Add a struct type to the type manager
    /// \param obj struct to add
    /// \return the ID for that struct
    static type_id_t AddStruct(const Struct& obj);

    /// Add a struct type to the type manager with a set ID
    /// \param id forced ID for that enum
    /// \param obj struct to add
    /// \return the ID for that struct
    /// \throw InvalidIdException if id is already assigned
    static type_id_t AddStruct(type_id_t id, const Struct& obj);

    /// Add an enum type to the type manager
    /// \param obj enum to add
    /// \return the ID for that enum
    static type_id_t AddEnum(const Enum& obj);

    /// Add an enum type to the type manager with a set ID
    /// \param id forced ID for that enum
    /// \param obj enum to add
    /// \return the ID for that enum
    /// \throw InvalidIdException if id is already assigned
    static type_id_t AddEnum(type_id_t id, const Enum& obj);


    /// Get a struct by its ID
    /// \param id the object id
    /// \return the struct definition
    /// \throw TypeNotFoundException if no type has been found
    static const Struct& GetStruct(type_id_t id);

    /// Get a struct by its ID
    /// \param name the name of the type
    /// \return the struct definition
    /// \throw TypeNotFoundException if no type has been found
    static const Struct& GetStruct(const std::string& name);

    /// Get a struct ID by its name
    /// \param name the name of the type
    /// \return the ID of the struct
    /// \throw TypeNotFoundException if no type has been found
    static type_id_t GetStructId(const std::string& name);

    /// Get list of Struct ids, including fundamentals
    /// \return list of stored struct ids
    static std::vector<type_id_t> GetStructIds();

    /// Get an enum by its ID
    /// \param name the name of the type
    /// \return the enum definition
    /// \throw TypeNotFoundException if no type has been found
    static const Enum& GetEnum(type_id_t id);

    /// Get an enum by its ID
    /// \param name the name of the type
    /// \return the enum definition
    /// \throw TypeNotFoundException if no type has been found
    static const Enum& GetEnum(const std::string& name);

    /// Get an enum ID by its name
    /// \param name the name of the type
    /// \return the ID of the enum
    /// \throw TypeNotFoundException if no type has been found
    static type_id_t GetEnumId(const std::string& name);

    /// Get list of enum ids
    /// \return list of stored enum ids
    static std::vector<type_id_t> GetEnumIds();

    /// Get the name of a type regardless of it being a struct or an enum
    /// \return the name of the type, or a warning reporting the id of the not found type
    static std::string GetTypeName(type_id_t id);

private:
    [[nodiscard]] bool IsIdTaken(type_id_t id);

    void        AddFundamentals();
    static void AddBuiltInEnums();
    static void AddBuiltInStructs();

private:    // Tests related
    [[maybe_unused]] static void ResetForTests();
    friend class TestTypeManager;
};
}    // namespace Frasy::Type


#endif    // INSTRUMENTATION_BOARD_MAIN_INTERFACES_COMMANDS_TYPE_MANAGER_H
