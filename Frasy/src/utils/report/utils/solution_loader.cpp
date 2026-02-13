/**
 * @file    solution_loader.cpp
 * @author  Paul Thomas
 * @date    3/17/2025
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

#include "solution_loader.h"

#include <Brigerad/Core/Log.h>
#include <filesystem>
#include <fstream>

namespace Frasy::Report::SolutionLoader {
nlohmann::json loadJson(const std::string& path)
{
    if (!std::filesystem::exists(path)) { return {}; }
    std::ifstream  ifs(path);
    nlohmann::json jObject {};
    if (ifs.is_open()) {
        ifs >> jObject;
        ifs.close();
    }
    else {
        BR_LOG_ERROR("KVP Report", "Unable to open JSON file '{}'", path);
    }
    return jObject;
}

Solution loadSolution()
{
    auto     json = loadJson("lua/solution.json");
    Solution solution;
    if (json.empty()) { return solution; }

    for (const auto& section : json) {
        for (const auto& sequenceList : section) {
            for (const auto& sequence : sequenceList) {
                BR_LOG_DEBUG("KVP Report", "Loading sequence '{}'", sequence["name"].get<std::string>());
                for (const auto& testGroup : sequence["tests"]) {
                    for (const auto& test : testGroup) {
                        BR_LOG_DEBUG("KVP Report", "Test '{}'", test.get<std::string>());
                        auto it = std::ranges::find_if(solution,
                                                       [name = sequence["name"].get<std::string>()](const auto& test) {
                                                           return test.first == name;
                                                       });
                        if (it != solution.end()) { it->second.push_back(test); }
                        else {
                            solution.emplace_back(sequence["name"], std::vector {test.get<std::string>()});
                        }
                    }
                }
            }
        }
    }

    return solution;
}
}    // namespace Frasy::Report::SolutionLoader
