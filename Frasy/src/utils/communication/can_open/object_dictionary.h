/**
 * @file    object_dictionary.h
 * @author  Paul Thomas
 * @date    2026-11-19
 * @brief   Description of the object dictionary of a CANopen device, loaded from its EDS or XDD file.
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
#ifndef FRASY_UTILS_COMMUNICATION_CAN_OPEN_OBJECT_DICTIONARY_H
#define FRASY_UTILS_COMMUNICATION_CAN_OPEN_OBJECT_DICTIONARY_H

#include "utils/communication/can_open/services/var_type.h"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Frasy::CanOpen {

//! Object codes, as defined by CiA 301 (and used by the `ObjectType` key of an EDS).
enum class OdObjectType : uint8_t {
    Null      = 0x00,
    Domain    = 0x02,
    DefType   = 0x05,
    DefStruct = 0x06,
    Var       = 0x07,
    Array     = 0x08,
    Record    = 0x09,
};

enum class OdAccess : uint8_t {
    None,         //!< The object cannot be accessed.
    ReadOnly,     //!< The value can change, but only the device can change it.
    WriteOnly,    //!< The value can be changed, but not read back.
    ReadWrite,
    Const,        //!< The value never changes.
};

[[nodiscard]] constexpr bool isReadable(OdAccess access) noexcept
{
    return access == OdAccess::ReadOnly || access == OdAccess::ReadWrite || access == OdAccess::Const;
}

[[nodiscard]] constexpr bool isWritable(OdAccess access) noexcept
{
    return access == OdAccess::WriteOnly || access == OdAccess::ReadWrite;
}

[[nodiscard]] std::string_view toString(OdAccess access) noexcept;
[[nodiscard]] std::string_view toString(OdObjectType type) noexcept;

/**
 * @brief One value of the object dictionary: a variable, or one sub-index of an array or record.
 */
struct OdEntry {
    uint8_t     subIndex = 0;
    std::string name;
    //! How the value is interpreted. Types without a native representation are handled as raw data (Undefined).
    VarType     varType = VarType::Undefined;
    //! Name of the data type as written in the CiA 301 specification (e.g. "UNSIGNED32"), for display.
    std::string dataTypeName;
    //! Code of the data type, as defined by CiA 301. 0 when the file does not say.
    uint16_t dataType = 0;
    OdObjectType objectType = OdObjectType::Var;
    OdAccess    access = OdAccess::ReadOnly;
    std::string defaultValue;
    //! Comment of the XDD about this entry. Empty when there is none (an EDS has no such thing).
    std::string description;
    bool        isPdoMappable = false;
    //! Limits of the value, as written in the file. Empty when there are none.
    std::string lowLimit;
    std::string highLimit;
    std::optional<uint32_t> stringLengthMin;
};

struct OdObject {
    uint16_t     index = 0;
    std::string  name;
    OdObjectType type = OdObjectType::Var;
    std::string  description;
    //! Number of sub-indexes declared by the file. Arrays and records only.
    uint32_t subNumber = 0;
    //! A single entry (sub-index 0) for variables and domains, the sub-indexes for arrays and records.
    std::vector<OdEntry> entries;

    //! True when the object is a container of sub-indexes, as opposed to a value of its own.
    [[nodiscard]] bool hasSubIndexes() const noexcept
    {
        return type == OdObjectType::Array || type == OdObjectType::Record;
    }
};

class ObjectDictionary {
public:
    /**
     * @brief Loads the object dictionary described by a file.
     *
     * Both the INI-like EDS (CiA 306) and the XML based XDD (CiA 311) formats are supported. The format is detected
     * from the content of the file, not from its extension.
     *
     * @return The dictionary, or a message describing why the file could not be used.
     */
    [[nodiscard]] static std::expected<ObjectDictionary, std::string> load(const std::filesystem::path& path);

    /**
     * @brief Builds the dictionary from the content of an EDS or of a XDD, which is detected from the content.
     */
    [[nodiscard]] static std::expected<ObjectDictionary, std::string> parse(std::string_view text);
    [[nodiscard]] static std::expected<ObjectDictionary, std::string> parseEds(std::string_view text);
    [[nodiscard]] static std::expected<ObjectDictionary, std::string> parseXdd(std::string_view text);

    //! Objects sorted by index.
    [[nodiscard]] const std::vector<OdObject>& getObjects() const noexcept { return m_objects; }
    [[nodiscard]] const OdObject*              findObject(uint16_t index) const noexcept;
    [[nodiscard]] const std::string&           getVendorName() const noexcept { return m_vendorName; }
    [[nodiscard]] const std::string&           getProductName() const noexcept { return m_productName; }

private:
    std::vector<OdObject> m_objects;
    std::string           m_vendorName;
    std::string           m_productName;
};

}    // namespace Frasy::CanOpen

#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_OBJECT_DICTIONARY_H
