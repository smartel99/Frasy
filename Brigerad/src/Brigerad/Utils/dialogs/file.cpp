/**
 * @file    file.cpp
 * @author  Samuel Martel
 * @date    2023-05-04
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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "file.h"

#include <Brigerad/Utils/ranges.h>
#include <filesystem>
#include <tinyfiledialogs.h>

namespace Brigerad::Dialogs
{
namespace
{
std::vector<const char*> FlattenStrings(const std::vector<std::string>& strings)
{
    std::vector<const char*> flattened;
    flattened.reserve(strings.size());
    for (auto&& string : strings) { flattened.push_back(string.c_str()); }

    return flattened;
}
}    // namespace

std::optional<std::string> SaveFile(const std::string&              title,
                                    const std::string&              defaultPathAndFile,
                                    const std::vector<std::string>& filters,
                                    const std::string&              description)
{
    auto  filtersStrs = FlattenStrings(filters);
    char* selected    = tinyfd_saveFileDialog(title.c_str(),
                                           defaultPathAndFile.empty() ? std::filesystem::current_path().string().c_str()
                                                                         : defaultPathAndFile.c_str(),
                                           static_cast<int>(filtersStrs.size()),
                                           filtersStrs.data(),
                                           description.c_str());
    if (selected == nullptr) { return std::nullopt; }
    return selected;
}

std::optional<std::string> OpenFile(const std::string&              title,
                                    const std::string&              defaultPathAndFile,
                                    const std::vector<std::string>& filters,
                                    const std::string&              filterDescription)
{
    auto  filtersStrs = FlattenStrings(filters);
    char* selected    = tinyfd_openFileDialog(title.c_str(),
                                           defaultPathAndFile.empty() ? std::filesystem::current_path().string().c_str()
                                                                         : defaultPathAndFile.c_str(),
                                           static_cast<int>(filtersStrs.size()),
                                           filtersStrs.data(),
                                           filterDescription.c_str(),
                                           0);
    if (selected == nullptr) { return std::nullopt; }
    else { return selected; }
}

std::optional<std::vector<std::string>> OpenFiles(const std::string&              title,
                                                  const std::string&              defaultPathAndFile,
                                                  const std::vector<std::string>& filters,
                                                  const std::string&              filterDescription)
{
    auto  filtersStrs = FlattenStrings(filters);
    char* selected    = tinyfd_openFileDialog(title.c_str(),
                                           defaultPathAndFile.empty() ? std::filesystem::current_path().string().c_str()
                                                                         : defaultPathAndFile.c_str(),
                                           static_cast<int>(filtersStrs.size()),
                                           filtersStrs.data(),
                                           filterDescription.c_str(),
                                           1);
    if (selected == nullptr) { return std::nullopt; }
    else
    {
        // Files are separated by '|'.
        using namespace std::string_view_literals;
        return std::ranges::views::split(std::string_view(selected), "|"sv) | Brigerad::To<std::vector<std::string>>();
    }
}

std::optional<std::string> OpenFolder(const std::string& title, const std::string& defaultPathAndFile)
{
    char* selected = tinyfd_selectFolderDialog(
      title.c_str(),
      defaultPathAndFile.empty() ? std::filesystem::current_path().string().c_str() : defaultPathAndFile.c_str());
    if (selected == nullptr) { return std::nullopt; }
    else { return selected; }
}
}    // namespace Brigerad::Dialogs
