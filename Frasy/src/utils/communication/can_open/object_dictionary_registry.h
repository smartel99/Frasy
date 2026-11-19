/**
 * @file    object_dictionary_registry.h
 * @author  Paul Thomas
 * @date    2026-11-19
 * @brief   Keeps the object dictionaries that were read from EDS and XDD files, so that each file is parsed once.
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
#ifndef FRASY_UTILS_COMMUNICATION_CAN_OPEN_OBJECT_DICTIONARY_REGISTRY_H
#define FRASY_UTILS_COMMUNICATION_CAN_OPEN_OBJECT_DICTIONARY_REGISTRY_H

#include "utils/communication/can_open/object_dictionary.h"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace Frasy::CanOpen {

/**
 * @brief Parses the EDS and XDD files, once, for everybody that needs them.
 *
 * The Lua environment, which builds its own table out of the dictionary, and the CANopen viewer both ask for the
 * dictionary of a board: the file is read and parsed by the first one, the others share the result. A file is read
 * again only if it changed on disk, or when asked to.
 *
 * Thread-safe. A global entry point is needed because the Lua bindings and the GUI have no common owner.
 */
class ObjectDictionaryRegistry {
public:
    using Result = std::expected<std::shared_ptr<const ObjectDictionary>, std::string>;

    [[nodiscard]] static ObjectDictionaryRegistry& get();

    /**
     * @brief Gives the dictionary described by a file.
     *
     * @param path EDS or XDD file.
     * @param forceReload Read the file again, even if it did not change.
     * @return The dictionary, or why it could not be loaded. Failures are kept too, so that a broken file is not parsed
     * again and again.
     */
    [[nodiscard]] Result load(const std::filesystem::path& path, bool forceReload = false);

    //! Forgets everything that was loaded.
    void clear();

private:
    struct Entry {
        std::filesystem::file_time_type lastWrite;
        std::uintmax_t                  size = 0;
        Result                          result;
    };

    ObjectDictionaryRegistry() = default;

    std::mutex                m_mutex;
    std::map<std::string, Entry> m_entries;
};

}    // namespace Frasy::CanOpen

#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_OBJECT_DICTIONARY_REGISTRY_H
