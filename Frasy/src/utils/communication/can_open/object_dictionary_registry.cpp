/**
 * @file    object_dictionary_registry.cpp
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
#include "object_dictionary_registry.h"

#include <format>

namespace Frasy::CanOpen {

ObjectDictionaryRegistry& ObjectDictionaryRegistry::get()
{
    static ObjectDictionaryRegistry s_instance;
    return s_instance;
}

ObjectDictionaryRegistry::Result ObjectDictionaryRegistry::load(const std::filesystem::path& path, bool forceReload)
{
    std::error_code ec;
    const auto      absolute = std::filesystem::absolute(path, ec);
    const auto      key      = (ec ? path : absolute).lexically_normal().generic_string();

    // The lock is held while parsing: a second caller waits for the result instead of parsing the file again.
    std::lock_guard lock {m_mutex};

    const auto lastWrite = std::filesystem::last_write_time(path, ec);
    const auto size      = ec ? std::uintmax_t {0} : std::filesystem::file_size(path, ec);
    if (ec) {
        m_entries.erase(key);
        return std::unexpected {std::format("Unable to open '{}': {}", path.string(), ec.message())};
    }

    if (auto it = m_entries.find(key); it != m_entries.end() && !forceReload && it->second.lastWrite == lastWrite &&
                                       it->second.size == size) {
        return it->second.result;
    }

    auto   parsed = ObjectDictionary::load(path);
    Result result = parsed.has_value() ? Result {std::make_shared<const ObjectDictionary>(std::move(*parsed))}
                                       : Result {std::unexpected {std::move(parsed.error())}};
    m_entries.insert_or_assign(key, Entry {.lastWrite = lastWrite, .size = size, .result = result});
    return result;
}

void ObjectDictionaryRegistry::clear()
{
    std::lock_guard lock {m_mutex};
    m_entries.clear();
}

}    // namespace Frasy::CanOpen
